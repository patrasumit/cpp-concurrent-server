#pragma once

#include <chrono>
#include <string>

enum class ReceiveResult
{
    Success,
    ClientClosed,
    ReceiveError,
    RequestTooLarge,
    BadRequest,
    Timeout
};

class Connection
{
public:
    explicit Connection(
        int client_fd,
        std::chrono::milliseconds receive_timeout =
            std::chrono::seconds(10)
    );
    ~Connection();

    Connection(const Connection&) = delete; // Delete the copy constructor
    Connection& operator=(const Connection&) = delete; // Delete the copy assignment operator

    Connection(Connection&& other) noexcept; // Move Constructor
    Connection& operator=(Connection&& other) noexcept; // Move Assignment Operartor

    ReceiveResult receiveRequest(std::string& raw_request);

    bool sendResponse(const std::string& raw_response);

private:
    int client_fd;
    std::string receive_buffer;
};