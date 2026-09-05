#include "networking/Connection.h"

#include <gtest/gtest.h>

#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <chrono>

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

TEST(Connection, ClientClosed)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    Connection connection(fds[1]);

    close(fds[0]);

    std::string received_request;

    ReceiveResult result =
        connection.receiveRequest(received_request);

    EXPECT_EQ(result, ReceiveResult::ClientClosed);
}

TEST(Connection, ReceiveTimeout)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    Connection connection(
        fds[1],
        std::chrono::milliseconds(100)
    );

    std::string received_request;

    ReceiveResult result =
        connection.receiveRequest(received_request);

    EXPECT_EQ(result, ReceiveResult::Timeout);

    close(fds[0]);
}

TEST(Connection, RequestTooLarge)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    const std::string request =
        "POST /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 2000000\r\n"
        "\r\n";

    ASSERT_EQ(
        send(
            fds[0],
            request.data(),
            request.size(),
            0
        ),
        static_cast<ssize_t>(request.size())
    );

    Connection connection(fds[1]);

    std::string received_request;

    ReceiveResult result =
        connection.receiveRequest(received_request);

    EXPECT_EQ(result, ReceiveResult::RequestTooLarge);

    close(fds[0]);
}

TEST(Connection, InvalidContentLength)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    const std::string request =
        "POST /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: abc\r\n"
        "\r\n";

    ASSERT_EQ(
        send(
            fds[0],
            request.data(),
            request.size(),
            0
        ),
        static_cast<ssize_t>(request.size())
    );

    Connection connection(fds[1]);

    std::string received_request;

    ReceiveResult result =
        connection.receiveRequest(received_request);

    EXPECT_EQ(result, ReceiveResult::BadRequest);

    close(fds[0]);
}

TEST(Connection, SendResponse)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    Connection connection(fds[0]);

    const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 6\r\n"
        "\r\n"
        "Hello\n";

    EXPECT_TRUE(
        connection.sendResponse(response)
    );

    char buffer[1024]{};

    ssize_t bytes_received =
        recv(
            fds[1],
            buffer,
            sizeof(buffer),
            0
        );

    ASSERT_EQ(
        bytes_received,
        static_cast<ssize_t>(response.size())
    );

    std::string received(
        buffer,
        bytes_received
    );

    EXPECT_EQ(received, response);

    close(fds[1]);
}

TEST(Connection, DestructorClosesSocket)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    {
        Connection connection(fds[0]);
    }

    // fds[0] should now be closed.
    // Writing from the peer should fail because there is
    // no longer a connected peer.

    const char data = 'x';

    ssize_t result =
        send(
            fds[1],
            &data,
            1,
            MSG_NOSIGNAL
        );

    EXPECT_EQ(result, -1);
    EXPECT_TRUE(
        errno == EPIPE ||
        errno == ECONNRESET
    );

    close(fds[1]);
}

TEST(Connection, SendResponseFailure)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    Connection connection(fds[0]);

    close(fds[1]);

    const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    EXPECT_FALSE(
        connection.sendResponse(response)
    );
}

TEST(Connection, RejectsMalformedHeader)
{
    int fds[2];

    ASSERT_EQ(
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds),
        0
    );

    const std::string request =
        "GET /hello HTTP/1.1\r\n"
        "Host localhost\r\n"          // malformed: missing ':'
        "Connection: close\r\n"
        "\r\n";

    ASSERT_EQ(
        send(
            fds[0],
            request.data(),
            request.size(),
            0
        ),
        static_cast<ssize_t>(request.size())
    );

    Connection connection(fds[1]);

    std::string received_request;

    ReceiveResult result =
        connection.receiveRequest(received_request);

    EXPECT_EQ(
        result,
        ReceiveResult::BadRequest
    );

    close(fds[0]);
}
