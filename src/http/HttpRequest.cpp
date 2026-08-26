#include "http/HttpRequest.h"

#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

#include "http/HttpUtils.h"

bool parseRequest(
    const std::string& raw_request,
    HttpRequest& request)
{
    std::stringstream ss(raw_request);
    std::string line;

    // -------------------------
    // Request line
    // -------------------------

    if (!std::getline(ss, line))
    {
        return false;
    }

    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    std::stringstream request_line(line);

    if (!(request_line
            >> request.method
            >> request.path
            >> request.version))
    {
        return false;
    }

    std::string extra;

    if (request_line >> extra)
    {
        return false;
    }

    if (request.version != "HTTP/1.1")
    {
        return false;
    }

    // -------------------------
    // Headers
    // -------------------------

    while (std::getline(ss, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // Empty line = end of headers
        if (line.empty())
            break;

        size_t colon = line.find(':');

        if (colon == std::string::npos)
        {
            return false;
        }

        std::string name =
            line.substr(0, colon);

        std::string value =
            line.substr(colon + 1);

        if (!value.empty() && value.front() == ' ')
            value.erase(0, 1);

        request.headers[toLower(name)] = value;
    }

    // -------------------------
    // Body
    // -------------------------

    size_t header_end =
        raw_request.find("\r\n\r\n");

    if (header_end != std::string::npos)
    {
        request.body =
            raw_request.substr(header_end + 4);
    }

    return true;
}

bool shouldKeepAlive(const HttpRequest& request)
{
    auto it = request.headers.find("connection");

    if (it != request.headers.end())
    {
        if (it->second == "close")
        {
            return false;
        }
    }

    return request.version == "HTTP/1.1";
}