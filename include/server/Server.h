#pragma once

#include "threadpool/ThreadPool.h"

class Server
{
public:
    explicit Server(int port);
    ~Server();

    void run();
    void stop();

private:
    void handleClient(int client_fd);
    void setupSocket();
    // void setupSignalHandling();

    int server_fd{-1};
    int port;

    ThreadPool pool;
};