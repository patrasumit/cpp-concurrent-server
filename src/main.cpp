#include <iostream>
#include <cstring>
#include <unistd.h>
#include <string>
#include <thread>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal> 
#include <cerrno>

#include "threadpool/ThreadPool.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/Router.h"
#include "networking/Connection.h"

volatile std::sig_atomic_t server_running = 1;

void handle_signal(int){
    server_running = 0;
}


void handle_client(int client_fd)
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

int main()
{
    // Server Setup

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        std::cout << "Socket creation failed\n";
        return 1;
    }

    int opt = 1;

    setsockopt(
        server_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );

    sockaddr_in server_address{};

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    if (bind(
            server_fd,
            (struct sockaddr*)&server_address,
            sizeof(server_address)) < 0)
    {
        std::cout << "Bind failed\n";
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        std::cout << "Listen failed\n";
        return 1;
    }

    // Signal handling

    struct sigaction sa{};

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);

    // Client Setup for accept

    ThreadPool pool(3);

    while (server_running)
    {
        sockaddr_in client_address{};
        socklen_t client_address_len =
            sizeof(client_address);

        int client_fd = accept(
            server_fd,
            (struct sockaddr*)&client_address,
            &client_address_len
        );

        if (client_fd < 0)
        {
            if (errno == EINTR && !server_running)
            {
                std::cout << "Shutdown requested\n";
                break;
            }

            std::cout << "Accept Failed!\n";
            continue;
        }

        if (!pool.enqueue([client_fd] {
                handle_client(client_fd);
            }))
        {
            close(client_fd);
        }
    }

    close(server_fd);

    std::cout << "Server shutting down...\n";

    return 0;
}
