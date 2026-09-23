#include "http/Router.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

Router::Router()
{
    registerBuiltinRoutes();
}

bool Router::get(
    const std::string& path,
    Handler handler)
{
    return addRoute("GET", path, std::move(handler));
}

bool Router::post(
    const std::string& path,
    Handler handler)
{
    return addRoute("POST", path, std::move(handler));
}

bool Router::put(
    const std::string& path,
    Handler handler)
{
    return addRoute("PUT", path, std::move(handler));
}

bool Router::patch(
    const std::string& path,
    Handler handler)
{
    return addRoute("PATCH", path, std::move(handler));
}

bool Router::del(
    const std::string& path,
    Handler handler)
{
    return addRoute("DELETE", path, std::move(handler));
}

bool Router::addRoute(
    const std::string& method,
    const std::string& path,
    Handler handler)
{
    // Application code cannot register server-owned routes.
    if (isReservedRoute(path))
    {
        return false;
    }

    auto& methodRoutes = routes[method];

    // Do not allow duplicate routes.
    if (methodRoutes.find(path) != methodRoutes.end())
    {
        return false;
    }

    methodRoutes[path] = std::move(handler);

    return true;
}

void Router::registerBuiltinRoutes()
{
    // These routes belong to the HTTP server library.
    // Applications cannot override them.

    routes["GET"]["/__server/hello"] =
        [](const HttpRequest&)
        {
            return HttpResponse{
                200,
                "OK",
                {{"Content-Type", "text/plain"}},
                "Hello from cpp-concurrent-server!\n"
            };
        };

    routes["GET"]["/__server/status"] =
        [](const HttpRequest&)
        {
            return HttpResponse{
                200,
                "OK",
                {{"Content-Type", "text/plain"}},
                "Server is running!\n"
            };
        };

    routes["GET"]["/__server/slow"] =
        [](const HttpRequest&)
        {
            std::cout << "Slow request: START\n";

            std::this_thread::sleep_for(
                std::chrono::seconds(5));

            std::cout << "Slow request: END\n";

            return HttpResponse{
                200,
                "OK",
                {{"Content-Type", "text/plain"}},
                "Slow request completed!\n"
            };
        };
}

bool Router::isReservedRoute(
    const std::string& path) const
{
    constexpr const char* reservedPrefix =
        "/__server/";

    return path.rfind(reservedPrefix, 0) == 0;
}

bool Router::pathExists(
    const std::string& path) const
{
    for (const auto& [method, pathRoutes] : routes)
    {
        if (pathRoutes.find(path) != pathRoutes.end())
        {
            return true;
        }
    }

    return false;
}

std::string Router::getAllowedMethods(
    const std::string& path) const
{
    std::string result;

    for (const auto& [method, pathRoutes] : routes)
    {
        if (pathRoutes.find(path) != pathRoutes.end())
        {
            if (!result.empty())
            {
                result += ", ";
            }

            result += method;
        }
    }

    return result;
}

HttpResponse Router::route(
    const HttpRequest& request) const
{
    auto methodIt = routes.find(request.method);

    if (methodIt != routes.end())
    {
        const auto& pathRoutes = methodIt->second;

        auto pathIt = pathRoutes.find(request.path);

        if (pathIt != pathRoutes.end())
        {
            return pathIt->second(request);
        }
    }

    // Path exists, but requested HTTP method does not.
    if (pathExists(request.path))
    {
        return HttpResponse{
            405,
            "Method Not Allowed",
            {
                {"Content-Type", "text/plain"},
                {"Allow", getAllowedMethods(request.path)}
            },
            "Method not allowed\n"
        };
    }

    // Neither path nor method exists.
    return HttpResponse{
        404,
        "Not Found",
        {
            {"Content-Type", "text/plain"}
        },
        "Resource not found\n"
    };
}