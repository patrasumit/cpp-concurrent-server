#include "http/HttpRequest.h"

#include <gtest/gtest.h>

TEST(HttpRequest, ValidGetRequest)
{
    const std::string raw_request =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    HttpRequest request;

    bool result =
        parseRequest(raw_request, request);

    EXPECT_TRUE(result);

    EXPECT_EQ(request.method, "GET");
    EXPECT_EQ(request.path, "/hello");
    EXPECT_EQ(request.version, "HTTP/1.1");
    EXPECT_EQ(request.headers.at("host"), "localhost");
    EXPECT_TRUE(request.body.empty());
}

TEST(HttpRequest, MalformedRequestLine)
{
    const std::string raw_request =
        "GET /hello\r\n"
        "Host: localhost\r\n"
        "\r\n";

    HttpRequest request;

    bool result =
        parseRequest(raw_request, request);

    EXPECT_FALSE(result);
}

TEST(HttpRequest, PostRequest)
{
    const std::string raw_request =
        "POST /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World";

    HttpRequest request;

    bool result =
        parseRequest(raw_request, request);

    EXPECT_TRUE(result);

    EXPECT_EQ(request.method, "POST");
    EXPECT_EQ(request.path, "/hello");
    EXPECT_EQ(request.version, "HTTP/1.1");
    EXPECT_EQ(request.headers.at("host"), "localhost");
    EXPECT_EQ(request.headers.at("content-length"), "11");
    EXPECT_EQ(request.body, "Hello World");
}

TEST(HttpRequest, UnsupportedHttpVersion)
{
    const std::string raw_request =
        "GET /hello HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "\r\n";

    HttpRequest request;

    bool result =
        parseRequest(raw_request, request);

    EXPECT_FALSE(result);
}

TEST(HttpRequest, MalformedHeader)
{
    const std::string raw_request =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "InvalidHeader\r\n"
        "\r\n";

    HttpRequest request;

    bool result =
        parseRequest(raw_request, request);

    EXPECT_FALSE(result);
}

TEST(HttpRequest, DefaultKeepAlive)
{
    HttpRequest request;

    request.version = "HTTP/1.1";

    EXPECT_TRUE(shouldKeepAlive(request));
}

TEST(HttpRequest, ConnectionClose)
{
    HttpRequest request;

    request.version = "HTTP/1.1";
    request.headers["connection"] = "close";

    EXPECT_FALSE(shouldKeepAlive(request));
}
