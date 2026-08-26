#include "http/Router.h"

#include <gtest/gtest.h>

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
