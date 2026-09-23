#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"

#include <functional>
#include <string>
#include <unordered_map>

class Router
{
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    Router();

    bool get(const std::string& path, Handler handler);
    bool post(const std::string& path, Handler handler);
    bool put(const std::string& path, Handler handler);
    bool patch(const std::string& path, Handler handler);
    bool del(const std::string& path, Handler handler);

    HttpResponse route(const HttpRequest& request) const;

private:
    using RouteTable =
        std::unordered_map<
            std::string,
            std::unordered_map<std::string, Handler>
        >;

    RouteTable routes;

    void registerBuiltinRoutes();

    bool addRoute(
        const std::string& method,
        const std::string& path,
        Handler handler
    );

    bool isReservedRoute(const std::string& path) const;

    bool pathExists(const std::string& path) const;

    std::string getAllowedMethods(
        const std::string& path
    ) const;
};