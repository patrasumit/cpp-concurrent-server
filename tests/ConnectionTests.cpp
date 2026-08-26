#include "networking/Connection.h"

#include <gtest/gtest.h>

#include <sys/socket.h>
#include <unistd.h>

TEST(Connection, ReceiveRequest)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    const std::string raw_request =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ssize_t bytes_sent =
        send(
            fds[0],
            raw_request.data(),
            raw_request.size(),
            0
        );

    ASSERT_EQ(
        bytes_sent,
        static_cast<ssize_t>(raw_request.size())
    );

    Connection connection(fds[1]);

    std::string received_request;

    ReceiveResult result =
        connection.receiveRequest(received_request);

    EXPECT_EQ(result, ReceiveResult::Success);
    EXPECT_EQ(received_request, raw_request);

    close(fds[0]);
}

TEST(Connection, ReceivePartialBody)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    const std::string request_part1 =
        "POST /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello";

    const std::string request_part2 =
        " World";

    ASSERT_EQ(
        send(
            fds[0],
            request_part1.data(),
            request_part1.size(),
            0
        ),
        static_cast<ssize_t>(request_part1.size())
    );

    ASSERT_EQ(
        send(
            fds[0],
            request_part2.data(),
            request_part2.size(),
            0
        ),
        static_cast<ssize_t>(request_part2.size())
    );

    Connection connection(fds[1]);

    std::string received_request;

    ReceiveResult result =
        connection.receiveRequest(received_request);

    EXPECT_EQ(result, ReceiveResult::Success);

    EXPECT_EQ(
        received_request,
        request_part1 + request_part2
    );

    close(fds[0]);
}

TEST(Connection, MultipleRequests)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    const std::string request1 =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    const std::string request2 =
        "GET /status HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    const std::string combined =
        request1 + request2;

    ASSERT_EQ(
        send(
            fds[0],
            combined.data(),
            combined.size(),
            0
        ),
        static_cast<ssize_t>(combined.size())
    );

    Connection connection(fds[1]);

    std::string received_request;

    ReceiveResult result =
        connection.receiveRequest(received_request);

    ASSERT_EQ(result, ReceiveResult::Success);
    EXPECT_EQ(received_request, request1);

    received_request.clear();

    result =
        connection.receiveRequest(received_request);

    EXPECT_EQ(result, ReceiveResult::Success);
    EXPECT_EQ(received_request, request2);

    close(fds[0]);
}
