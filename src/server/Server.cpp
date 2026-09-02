#include "server/Server.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/Router.h"
#include "networking/Connection.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <csignal>
#include <cerrno>
#include <cstring>

namespace
{
    volatile std::sig_atomic_t server_running = 1;

    void handle_signal(int)
    {
        server_running = 0;
    }
}

Server::Server(int port)
    : port(port),
      pool(3)
{
    setupSocket();
}

Server::~Server()
{
    if (server_fd >= 0)
    {
        close(server_fd);
    }
}

void Server::run()
{
    struct sigaction sa{};

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);

    while (server_running)
    {
        sockaddr_in client_address{};
        socklen_t client_address_len =
            sizeof(client_address);

        int client_fd = accept(
            server_fd,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_address_len
        );

        if (client_fd < 0)
        {
            if (errno == EINTR && !server_running)
            {
                std::cout << "Shutdown requested\n";
                break;
            }

            std::cout << "Accept failed!\n";
            continue;
        }

        if (!pool.enqueue([this, client_fd]
            {
                handleClient(client_fd);
            }))
        {
            close(client_fd);
        }
    }
}

void Server::stop()
{
    server_running = 0;

    if (server_fd >= 0)
    {
        shutdown(server_fd, SHUT_RDWR);
    }
}

void Server::setupSocket()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        throw std::runtime_error("Socket creation failed");
    }

    int opt = 1;

    if (setsockopt(
            server_fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)) < 0)
    {
        close(server_fd);
        server_fd = -1;

        throw std::runtime_error("setsockopt failed");
    }

    sockaddr_in server_address{};

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if (inet_pton(
            AF_INET,
            "127.0.0.1",
            &server_address.sin_addr) <= 0)
    {
        close(server_fd);
        server_fd = -1;

        throw std::runtime_error("Invalid server address");
    }

    if (bind(
            server_fd,
            reinterpret_cast<sockaddr*>(&server_address),
            sizeof(server_address)) < 0)
    {
        close(server_fd);
        server_fd = -1;

        throw std::runtime_error(
            std::string("bind failed: ") + std::strerror(errno)
        );
    }

    if (listen(server_fd, 10) < 0)
    {
        close(server_fd);
        server_fd = -1;

        throw std::runtime_error("Listen failed");
    }
}

void Server::handleClient(int client_fd)
{
    Connection connection(client_fd);

    while (true)
    {
        std::string raw_request;

        ReceiveResult result =
            connection.receiveRequest(raw_request);

        switch (result)
        {
            case ReceiveResult::Success:
                break;

            case ReceiveResult::ClientClosed:
                return;

            case ReceiveResult::ReceiveError:
                return;

            case ReceiveResult::RequestTooLarge:
            {
                HttpResponse response{
                    413,
                    "Content Too Large",
                    {
                        {"Content-type", "text/plain"},
                        {"Connection", "close"}
                    },
                    "Request too large\n"
                };

                connection.sendResponse(
                    serializeResponse(response)
                );

                return;
            }

            case ReceiveResult::BadRequest:
            {
                HttpResponse response{
                    400,
                    "Bad Request",
                    {
                        {"Content-type", "text/plain"},
                        {"Connection", "close"}
                    },
                    "Bad request\n"
                };

                connection.sendResponse(
                    serializeResponse(response)
                );

                return;
            }

            case ReceiveResult::Timeout:
            {   
                std::cout << "Client receive timeout\n";
                return;
            }
        }

        HttpRequest request;

        if (!parseRequest(raw_request, request))
        {
            HttpResponse response = {
                400,
                "Bad Request",
                {
                    {"Content-type", "text/plain"},
                    {"Connection", "close"}
                },
                "Bad request\n"
            };

            connection.sendResponse(
                serializeResponse(response)
            );

            break;
        }

        HttpResponse response =
            routeRequest(request);

        bool keep_alive =
            shouldKeepAlive(request);

        if (keep_alive)
        {
            response.headers["Connection"] =
                "keep-alive";
        }
        else
        {
            response.headers["Connection"] =
                "close";
        }

        std::string raw_response =
            serializeResponse(response);

        if (!connection.sendResponse(raw_response))
            break;

        if (!keep_alive)
            break;
    }
}

