#include "networking/Connection.h"

#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <utility>
#include <algorithm>
#include <cctype>
#include <sys/time.h>

#include "http/HttpUtils.h"

// Maximum Size of Request that we need to handle
constexpr size_t MAX_REQUEST_SIZE = 1024 * 1024; // 1 MB

Connection::Connection(int client_fd)
    : client_fd(client_fd)
{
    timeval timeout{};
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt(
            client_fd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            &timeout,
            sizeof(timeout)) < 0)
    {
        std::cout << "Failed to set receive timeout\n";
    }
}

Connection::Connection(Connection&& other) noexcept
    : client_fd(other.client_fd),
      receive_buffer(std::move(other.receive_buffer))
{
    other.client_fd = -1;
}

Connection& Connection::operator=(Connection&& other) noexcept
{
    if (this != &other)
    {
        if (client_fd != -1)
            close(client_fd);

        client_fd = other.client_fd;
        receive_buffer = std::move(other.receive_buffer);

        other.client_fd = -1;
    }

    return *this;
}

ReceiveResult Connection::receiveRequest(std::string& raw_request)
{
    char buffer[4096];

    while (true)
    {
        size_t header_end =
            receive_buffer.find("\r\n\r\n");

        if (header_end == std::string::npos)
        {
            int bytes_received = recv(
                client_fd,
                buffer,
                sizeof(buffer),
                0
            );

            if (bytes_received < 0)
            {
                if (errno == EINTR)
                    continue;

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    std::cout << "Receive timeout\n";
                    return ReceiveResult::Timeout;
                }

                std::cout << "Receive Failed\n";
                return ReceiveResult::ReceiveError;
            }

            if (bytes_received == 0)
            {
                std::cout << "Client closed connection\n";
                return ReceiveResult::ClientClosed;
            }

            receive_buffer.append(
                buffer,
                bytes_received
            );

            if (receive_buffer.size() > MAX_REQUEST_SIZE)
            {
                std::cout << "Request too large\n";
                return ReceiveResult::RequestTooLarge;
            }

            continue;
        }

        std::string headers =
            receive_buffer.substr(0, header_end);

        std::stringstream ss(headers);
        std::string line;

        size_t content_length = 0;

        while (std::getline(ss, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            size_t colon = line.find(':');

            if (colon == std::string::npos)
                continue;

            std::string name =
                toLower(line.substr(0, colon));

            std::string value =
                line.substr(colon + 1);

            if (!value.empty() && value.front() == ' ')
                value.erase(0, 1);

            if (name == "content-length")
            {
                try
                {
                    content_length = std::stoul(value);
                }
                catch (const std::exception&)
                {
                    std::cout << "Invalid Content-Length\n";
                    return ReceiveResult::BadRequest;
                }
            }

            std::cout << "Header: [" << name << "] = [" << value << "]\n";
            std::cout << "Content-Length = " << content_length << "\n";

        }

        size_t body_start =
            header_end + 4;

        size_t request_size =
            body_start + content_length;

        if (request_size > MAX_REQUEST_SIZE)
        {
            std::cout << "Request too large\n";
            return ReceiveResult::RequestTooLarge;
        }

        size_t body_received =
            receive_buffer.size() - body_start;

        while (body_received < content_length)
        {
            int bytes_received = recv(
                client_fd,
                buffer,
                sizeof(buffer),
                0
            );

            if (bytes_received < 0)
            {
                if (errno == EINTR)
                    continue;

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    std::cout << "Receive timeout\n";
                    return ReceiveResult::Timeout;
                }

                std::cout << "Receive Failed\n";
                return ReceiveResult::ReceiveError;
            }

            if (bytes_received == 0)
            {
                std::cout << "Client closed connection\n";
                return ReceiveResult::ClientClosed;
            }

            receive_buffer.append(
                buffer,
                bytes_received
            );

            body_received =
                receive_buffer.size() - body_start;
        }

        raw_request =
            receive_buffer.substr(
                0,
                request_size
            );

        receive_buffer.erase(
            0,
            request_size
        );

        return ReceiveResult::Success;
    }
}

bool Connection::sendResponse(const std::string& raw_response)
{
    size_t total_sent = 0;

    while (total_sent < raw_response.size())
    {
        ssize_t bytes_sent = send(
            client_fd,
            raw_response.data() + total_sent,
            raw_response.size() - total_sent,
            0
        );

        if (bytes_sent < 0)
        {
            if (errno == EINTR)
                continue;

            std::cout << "Send failed\n";
            return false;
        }

        if (bytes_sent == 0)
        {
            std::cout << "Connection closed while sending\n";
            return false;
        }

        total_sent += bytes_sent;
    }

    return true;
}

Connection::~Connection()
{
    if (client_fd != -1)
    {
        close(client_fd);
        std::cout << "Closing client connection\n";
    }
}