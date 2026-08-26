#pragma once

#include <string>
#include <unordered_map>

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string version;

    std::unordered_map<std::string, std::string> headers;

    std::string body;
};

// HttpRequest parseRequest(const std::string& raw_request); //// To check the correctness of the request while this might just pass for parsing further without checking

bool parseRequest(
    const std::string& raw_request,
    HttpRequest& request
);

bool shouldKeepAlive(const HttpRequest& request);
