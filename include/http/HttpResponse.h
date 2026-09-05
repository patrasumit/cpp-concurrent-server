#pragma once

#include <string>
#include <unordered_map>
#include "http/HttpUtils.h"

struct HttpResponse
{
    int statusCode;
    std::string statusText;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

std::string serializeResponse(const HttpResponse& response);