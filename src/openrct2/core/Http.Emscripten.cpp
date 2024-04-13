/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#if !defined(DISABLE_HTTP) && defined(__EMSCRIPTEN__)

#    include "../Version.h"
#    include "../core/Console.hpp"
#    include "Http.h"

#    include <cstring>
#    include <memory>
#    include <stdexcept>
#    include <thread>
#    include <sstream>
#include <emscripten.h>


#    define OPENRCT2_USER_AGENT "OpenRCT2/" OPENRCT2_VERSION

extern "C" {
extern char __em_js____asyncjs__openrct2_http_response[];
EM_ASYNC_JS(void, openrct2_http_response, (const char* url_r, int url_size, const char* body_r, int body_size, int verb, const char* headers_r, int headers_size, char** response_body_out, int32_t* response_body_length, int32_t* status_code_out, char** headers_out), {
    console.log('Here I am!');
    console.log('Here I am2!');
    console.log(arguments);
    console.log('die die die', url_r, url_size, body_r, body_size, verb, headers_r, headers_size, response_body_out, response_body_length, status_code_out, headers_out);
    console.log('aghhh');
    const url = Module.UTF8ToString(url_r, url_size);
    const verbs = ['GET','POST','PUT'];
    const method = verbs[verb];
    const body = Module.HEAPU8.slice(body_r, body_size);
    const headerString = Module.UTF8ToString(headers_r, headers_size).split('\n');
    const headersMap = {};
    for(const line of headerString) {
        if (!line.length) {
            continue;
        }
        const idx = line.indexOf(':');
        headersMap[line.substring(0, idx)] = line.substring(idx + 2);
    }
    const request = {headers: headersMap, method, body};
    if (!body.length) {
        delete request.body;
    }
    console.log('Making request', request, url);
    const res = await fetch(url, request);
    const text = await res.text();
    console.log('Response got back!', res);
    console.log('Back into C!', text.length);
    const headers = Array.from(res.headers.entries()).map(([key, value]) => `${key}: ${value}`).join('\n');
    Module.setValue(response_body_out, Module.stringToNewUTF8(text), '*');
    Module.setValue(response_body_length, lengthBytesUTF8(text), 'i32');
    Module.setValue(status_code_out, res.status, 'i32');
    Module.setValue(headers_out, Module.stringToNewUTF8(headers), '*');
    console.log('hi! fired off request!');
});
}
namespace Http
{
    Response Do(const Request& req)
    {
        printf("Time!!!\n");
        char* response_body_out = nullptr;
        int32_t response_body_length = 0;
        int32_t status_code_out = 0;
        char* headers_out = nullptr;

        printf("Doing an asynchronous HTTP call!\n");
        std::string headers;
        for (auto const& entry : req.header)
        {
            headers.append(entry.first);
            headers.append(": ");
            headers.append(entry.second);
            headers.append("\n");
        }
        
        openrct2_http_response(
            req.url.c_str(),
            req.url.size(),
            req.body.c_str(),
            req.body.size(),
            static_cast<int>(req.method),
            headers.c_str(),
            headers.size(),
            &response_body_out,
            &response_body_length,
            &status_code_out,
            &headers_out
        );

        Http::Response response;
        response.body = std::string(response_body_out, response_body_length);
        response.status = static_cast<Http::Status>(status_code_out);
        std::string headersString(headers_out);
        std::istringstream iss(headersString);
        std::string line;
        while (std::getline(iss, line))
        {
            // Do something with `line`
            auto pos = line.find(':');
            std::string key = line.substr(0, pos);
            // subtract 4 chars for ": " and "\r\n"
            std::string value = line.substr(pos + 2, line.size() - pos - 4);
            response.header[key] = value;
            std::transform(key.begin(), key.end(), key.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            if (key == "content-type") {
                response.content_type = value;
            }
        }
        free(headers_out);
        free(response_body_out);
        printf("Freed\n");
        printf("Body data: %lu %d\n", response.body.size(), response_body_length);
        return response;
    }

} // namespace Http

#endif
