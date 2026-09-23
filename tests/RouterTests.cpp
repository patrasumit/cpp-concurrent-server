#include <gtest/gtest.h>

#include "http/Router.h"

TEST(Router, GetHello)
{
    Router router;

    HttpRequest request{
        "GET",
        "/__server/hello",
        "HTTP/1.1",
        {},
        ""
    };

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.statusCode, 200);
    EXPECT_EQ(response.statusText, "OK");
    EXPECT_EQ(response.body,
              "Hello from cpp-concurrent-server!\n");
}

TEST(Router, MethodNotAllowed)
{
    Router router;

    HttpRequest request{
        "POST",
        "/__server/hello",
        "HTTP/1.1",
        {},
        ""
    };

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.statusCode, 405);
    EXPECT_EQ(response.statusText, "Method Not Allowed");

    auto it = response.headers.find("Allow");

    ASSERT_NE(it, response.headers.end());
    EXPECT_EQ(it->second, "GET");
}

TEST(Router, NotFound)
{
    Router router;

    HttpRequest request{
        "GET",
        "/does-not-exist",
        "HTTP/1.1",
        {},
        ""
    };

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.statusCode, 404);
    EXPECT_EQ(response.statusText, "Not Found");
}

TEST(Router, ApplicationCanRegisterRoute)
{
    Router router;

    bool registered = router.get(
        "/api/health",
        [](const HttpRequest&)
        {
            return HttpResponse{
                200,
                "OK",
                {{"Content-Type", "text/plain"}},
                "MyBrary is healthy\n"
            };
        });

    EXPECT_TRUE(registered);

    HttpRequest request{
        "GET",
        "/api/health",
        "HTTP/1.1",
        {},
        ""
    };

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.statusCode, 200);
    EXPECT_EQ(response.body, "MyBrary is healthy\n");
}

TEST(Router, DuplicateRouteIsRejected)
{
    Router router;

    EXPECT_TRUE(
        router.get(
            "/api/test",
            [](const HttpRequest&)
            {
                return HttpResponse{
                    200,
                    "OK",
                    {},
                    "first"
                };
            }));

    EXPECT_FALSE(
        router.get(
            "/api/test",
            [](const HttpRequest&)
            {
                return HttpResponse{
                    200,
                    "OK",
                    {},
                    "second"
                };
            }));
}

TEST(Router, ReservedRouteIsRejected)
{
    Router router;

    EXPECT_FALSE(
        router.get(
            "/__server/custom",
            [](const HttpRequest&)
            {
                return HttpResponse{
                    200,
                    "OK",
                    {},
                    "should not be allowed"
                };
            }));
}

TEST(Router, BuiltinStatusRouteWorks)
{
    Router router;

    HttpRequest request{
        "GET",
        "/__server/status",
        "HTTP/1.1",
        {},
        ""
    };

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.statusCode, 200);
    EXPECT_EQ(response.body,
              "Server is running!\n");
}

TEST(Router, BuiltinSlowRouteWorks)
{
    Router router;

    HttpRequest request{
        "GET",
        "/__server/slow",
        "HTTP/1.1",
        {},
        ""
    };

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.statusCode, 200);
    EXPECT_EQ(response.body,
              "Slow request completed!\n");
}