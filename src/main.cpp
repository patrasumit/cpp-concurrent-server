#include "server/Server.h"
#include "logging/Logger.h"

int main()
{
    Logger::instance().info("Server starting");
    Server server(8080);
    server.run();
    return 0;
}