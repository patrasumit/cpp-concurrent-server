#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/Router.h"

#include <cassert>
#include <iostream>

void testValidGetRequest()
{
    const std::string raw_request =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    HttpRequest request;

    bool result =
        parseRequest(raw_request, request);

    assert(result);
    assert(request.method == "GET");
    assert(request.path == "/hello");
    assert(request.version == "HTTP/1.1");
    assert(request.headers.at("host") == "localhost");
    assert(request.body.empty());
}

void testMalformedRequestLine()
{
    const std::string raw_request =
        "GET /hello\r\n"
        "Host: localhost\r\n"
        "\r\n";

    HttpRequest request;

    bool result =
        parseRequest(raw_request, request);

    assert(!result);
}

void testResponseSerialization()
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

    assert(
        raw_response.find("HTTP/1.1 200 OK\r\n")
        != std::string::npos
    );

    assert(
        raw_response.find("Content-type: text/plain\r\n")
        != std::string::npos
    );

    assert(
        raw_response.find("Connection: keep-alive\r\n")
        != std::string::npos
    );

    assert(
        raw_response.find("Content-Length: 15\r\n")
        != std::string::npos
    );

    assert(
        raw_response.find("\r\n\r\nHello, client!\n")
        != std::string::npos
    );
}

void testPostRequest()
{
    const std::string raw_request =
        "POST /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    HttpRequest request;

    bool result =
        parseRequest(raw_request, request);

    assert(result);
    assert(request.method == "POST");
    assert(request.path == "/hello");
    assert(request.version == "HTTP/1.1");
    assert(request.headers.at("host") == "localhost");
    assert(request.body == "Hello");
}

void testUnsupportedHttpVersion()
{
    const std::string raw_request =
        "GET /hello HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "\r\n";

    HttpRequest request;

    bool result =
        parseRequest(raw_request, request);

    assert(!result);
}

void testMalformedHeader()
{
    const std::string raw_request =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "InvalidHeader\r\n"
        "\r\n";

    HttpRequest request;

    bool result =
        parseRequest(raw_request, request);

    assert(!result);
}

void testDefaultKeepAlive()
{
    HttpRequest request;

    request.version = "HTTP/1.1";

    assert(shouldKeepAlive(request));
}

void testConnectionClose()
{
    HttpRequest request;

    request.version = "HTTP/1.1";
    request.headers["connection"] = "close";

    assert(!shouldKeepAlive(request));
}

void testRouteGetHello()
{
    HttpRequest request;

    request.method = "GET";
    request.path = "/hello";
    request.version = "HTTP/1.1";

    HttpResponse response =
        routeRequest(request);

    assert(response.statusCode == 200);
    assert(response.statusText == "OK");
    assert(response.body == "Hello, client!\n");
}

void testRouteMethodNotAllowed()
{
    HttpRequest request;

    request.method = "PUT";
    request.path = "/hello";
    request.version = "HTTP/1.1";

    HttpResponse response =
        routeRequest(request);

    assert(response.statusCode == 405);
    assert(response.statusText == "Method Not Allowed");
    assert(response.headers.at("Allow").find("GET") != std::string::npos);
    assert(response.headers.at("Allow").find("POST") != std::string::npos);
}

void testRouteNotFound()
{
    HttpRequest request;

    request.method = "GET";
    request.path = "/does-not-exist";
    request.version = "HTTP/1.1";

    HttpResponse response =
        routeRequest(request);

    assert(response.statusCode == 404);
    assert(response.statusText == "Not Found");
    assert(response.body == "Resource not found\n");
}

int main()
{
    testValidGetRequest();
    testMalformedRequestLine();
    testResponseSerialization();
    testPostRequest();
    testUnsupportedHttpVersion();
    testMalformedHeader();
    testDefaultKeepAlive();
    testConnectionClose();
    testRouteGetHello();
    testRouteMethodNotAllowed();
    testRouteNotFound();

    std::cout << "All tests passed!\n";
}