#include "http/HttpResponse.h"

std::string serializeResponse(const HttpResponse& response)
{
    std::string result =
        "HTTP/1.1 " +
        std::to_string(response.statusCode) + " " +
        response.statusText + "\r\n";

    for (const auto& [name, value] : response.headers)
    {
        result += name + ": " + value + "\r\n";
    }

    result +=
        "Content-Length: " +
        std::to_string(response.body.size()) +
        "\r\n";

    result += "\r\n";

    result += response.body;

    return result;
}