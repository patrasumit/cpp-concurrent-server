#include <gtest/gtest.h>

#include "server/Server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <thread>
#include <chrono>

TEST(Server, HandlesGetRequestEndToEnd)
{
    constexpr int port = 18080;

    Server server(port);

    std::thread server_thread([&server]()
    {
        server.run();
    });

    // Give the server a moment to start listening.
    std::this_thread::sleep_for(
        std::chrono::milliseconds(50)
    );

    int client_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    ASSERT_GE(client_fd, 0);

    sockaddr_in server_address{};

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    ASSERT_EQ(
        inet_pton(
            AF_INET,
            "127.0.0.1",
            &server_address.sin_addr
        ),
        1
    );

    ASSERT_EQ(
        connect(
            client_fd,
            reinterpret_cast<sockaddr*>(&server_address),
            sizeof(server_address)
        ),
        0
    );

    const std::string request =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";

    ASSERT_EQ(
        send(
            client_fd,
            request.data(),
            request.size(),
            0
        ),
        static_cast<ssize_t>(request.size())
    );

    char buffer[4096]{};

    ssize_t bytes_received =
        recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    ASSERT_GT(bytes_received, 0);

    std::string response(
        buffer,
        static_cast<size_t>(bytes_received)
    );

    EXPECT_NE(
        response.find("200"),
        std::string::npos
    );

    EXPECT_NE(
        response.find("Hello"),
        std::string::npos
    );

    close(client_fd);

    // Stop the server.
    server.stop();

    server_thread.join();
}