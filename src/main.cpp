#include "server/Server.h"
#include "logging/Logger.h"

int main()
{
    Router router;
    Logger::instance().info("Server starting");
    Server server(8080, router);
    server.run();
    return 0;
}