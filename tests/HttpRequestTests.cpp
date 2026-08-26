#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/Router.h"
#include "networking/Connection.h"

#include <gtest/gtest.h>

#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <iostream>

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

TEST(Router, GetHello)
{
    HttpRequest request;

    request.method = "GET";
    request.path = "/hello";
    request.version = "HTTP/1.1";

    HttpResponse response =
        routeRequest(request);

    EXPECT_EQ(response.statusCode, 200);
    EXPECT_EQ(response.statusText, "OK");
    EXPECT_EQ(response.body, "Hello, client!\n");
}

TEST(Router, MethodNotAllowed)
{
    HttpRequest request;

    request.method = "PUT";
    request.path = "/hello";
    request.version = "HTTP/1.1";

    HttpResponse response =
        routeRequest(request);

    EXPECT_EQ(response.statusCode, 405);
    EXPECT_EQ(response.statusText, "Method Not Allowed");

    EXPECT_NE(
        response.headers.at("Allow").find("GET"),
        std::string::npos
    );

    EXPECT_NE(
        response.headers.at("Allow").find("POST"),
        std::string::npos
    );
}

TEST(Router, NotFound)
{
    HttpRequest request;

    request.method = "GET";
    request.path = "/does-not-exist";
    request.version = "HTTP/1.1";

    HttpResponse response =
        routeRequest(request);

    EXPECT_EQ(response.statusCode, 404);
    EXPECT_EQ(response.statusText, "Not Found");
    EXPECT_EQ(response.body, "Resource not found\n");
}

TEST(Connection, ReceiveRequest)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    const std::string raw_request =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ssize_t bytes_sent =
        send(
            fds[0],
            raw_request.data(),
            raw_request.size(),
            0
        );

    ASSERT_EQ(
        bytes_sent,
        static_cast<ssize_t>(raw_request.size())
    );

    Connection connection(fds[1]);

    std::string received_request;

    ReceiveResult result =
        connection.receiveRequest(received_request);

    EXPECT_EQ(result, ReceiveResult::Success);
    EXPECT_EQ(received_request, raw_request);

    close(fds[0]);
}

TEST(Connection, ReceivePartialBody)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    const std::string request_part1 =
        "POST /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello";

    const std::string request_part2 =
        " World";

    ASSERT_EQ(
        send(
            fds[0],
            request_part1.data(),
            request_part1.size(),
            0
        ),
        static_cast<ssize_t>(request_part1.size())
    );

    ASSERT_EQ(
        send(
            fds[0],
            request_part2.data(),
            request_part2.size(),
            0
        ),
        static_cast<ssize_t>(request_part2.size())
    );

    Connection connection(fds[1]);

    std::string received_request;

    ReceiveResult result =
        connection.receiveRequest(received_request);

    EXPECT_EQ(result, ReceiveResult::Success);

    EXPECT_EQ(
        received_request,
        request_part1 + request_part2
    );

    close(fds[0]);
}

TEST(Connection, MultipleRequests)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    const std::string request1 =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    const std::string request2 =
        "GET /status HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    const std::string combined =
        request1 + request2;

    ASSERT_EQ(
        send(
            fds[0],
            combined.data(),
            combined.size(),
            0
        ),
        static_cast<ssize_t>(combined.size())
    );

    Connection connection(fds[1]);

    std::string received_request;

    ReceiveResult result =
        connection.receiveRequest(received_request);

    ASSERT_EQ(result, ReceiveResult::Success);
    EXPECT_EQ(received_request, request1);

    received_request.clear();

    result =
        connection.receiveRequest(received_request);

    EXPECT_EQ(result, ReceiveResult::Success);
    EXPECT_EQ(received_request, request2);

    close(fds[0]);
}
