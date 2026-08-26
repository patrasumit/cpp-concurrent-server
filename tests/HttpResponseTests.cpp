#include "http/HttpResponse.h"

#include <gtest/gtest.h>

TEST(HttpResponse, Serialization)
{
    HttpResponse response{
        200,
        "OK",
        {
            {"Content-type", "text/plain"},
            {"Connection", "keep-alive"}
        },
        "Hello, client!\n"
    };

    std::string raw_response =
        serializeResponse(response);

    EXPECT_NE(
        raw_response.find("HTTP/1.1 200 OK\r\n"),
        std::string::npos
    );

    EXPECT_NE(
        raw_response.find("Content-type: text/plain\r\n"),
        std::string::npos
    );

    EXPECT_NE(
        raw_response.find("Connection: keep-alive\r\n"),
        std::string::npos
    );

    EXPECT_NE(
        raw_response.find("Content-Length: 15\r\n"),
        std::string::npos
    );

    EXPECT_NE(
        raw_response.find("\r\n\r\nHello, client!\n"),
        std::string::npos
    );
}