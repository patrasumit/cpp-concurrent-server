#include "http/Router.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <functional>
#include <unordered_map>
#include <string>
#include <vector>

using Handler = std::function<HttpResponse(const HttpRequest&)>;
using RouteKey = std::string;
using RouteTable =
    std::unordered_map<
        std::string,
        std::unordered_map<std::string, Handler>
    >;

// Route Table
RouteTable routes = {
    {
        "GET",
        {
            {
                "/",
                [](const HttpRequest&) {
                    return HttpResponse{
                        200,
                        "OK",
                        {
                            {"Content-type", "text/plain"}
                        },
                        "Welcome to the C++ Concurrent Server!\n"
                    };
                }
            },

            {
                "/hello",
                [](const HttpRequest&) {
                    return HttpResponse{
                        200,
                        "OK",
                        {
                            {"Content-type", "text/plain"}
                        },
                        "Hello, client!\n"
                    };
                }
            },

            {
                "/status",
                [](const HttpRequest&) {
                    return HttpResponse{
                        200,
                        "OK",
                        {
                            {"Content-type", "text/plain"}
                        },
                        "Server is running!\n"
                    };
                }
            },
            {
                "/slow",
                [](const HttpRequest&) {
                    std::cout << "Slow request: START\n";

                    std::this_thread::sleep_for(
                        std::chrono::seconds(5)
                    );

                    std::cout << "Slow request: END\n";

                    return HttpResponse{
                        200,
                        "OK",
                        {
                            {"Content-type", "text/plain"}
                        },
                        "Slow request completed!\n"
                    };
                }
            }
        }
    },
    {
        "POST",
        {
            {
                "/hello",
                [](const HttpRequest& request) {
                    std::cout << "POST body = [" << request.body << "]\n";
                    std::cout << "POST body size = " << request.body.size() << "\n";

                    return HttpResponse{
                        200,
                        "OK",
                        {
                            {"Content-type", "text/plain"}
                        },
                        "Received body: " + request.body + "\n"
                    };
                }
            }
        }
    }
};

// Helper Functions
bool pathExists(const std::string& path)
{
    for (const auto& [method, path_routes] : routes)
    {
        if (path_routes.find(path) != path_routes.end())
        {
            return true;
        }
    }

    return false;
}

std::string getAllowedMethods(const std::string& path)
{
    std::vector<std::string> allowed_methods;

    for (const auto& [method, path_routes] : routes)
    {
        if (path_routes.find(path) != path_routes.end())
        {
            allowed_methods.push_back(method);
        }
    }

    std::string result;

    for (size_t i = 0; i < allowed_methods.size(); ++i)
    {
        if (i > 0)
            result += ", ";

        result += allowed_methods[i];
    }

    return result;
}

HttpResponse routeRequest(const HttpRequest& request)
{
    // Find the requested HTTP method
    auto method_it = routes.find(request.method);

    if (method_it != routes.end())
    {
        // Get the routes registered for this method
        const auto& path_routes = method_it->second;

        // Find the requested path
        auto path_it = path_routes.find(request.path);

        if (path_it != path_routes.end())
        {
            // Execute the handler
            return path_it->second(request);
        }
    }

    // Method Not Allowed
    if (pathExists(request.path))
    {
        return {
            405,
            "Method Not Allowed",
            {
                {"Content-type", "text/plain"},
                {"Allow", getAllowedMethods(request.path)}
            },
            "Method not allowed\n"
        };
    }

    return {
        404,
        "Not Found",
        {
            {"Content-type", "text/plain"}
        },
        "Resource not found\n"
    };
}