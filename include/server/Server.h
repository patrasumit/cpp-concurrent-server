#pragma once

#include <atomic>
#include "threadpool/ThreadPool.h"
#include "http/Router.h"

class Server
{
public:
    explicit Server(int port, Router& router);
    ~Server();

    void run();
    void stop();

private:
    void handleClient(int client_fd);
    void setupSocket();
    // void setupSignalHandling();

    int server_fd{-1};
    int port;

    Router& router;

    ThreadPool pool;

    std::atomic<bool> running{true};
};