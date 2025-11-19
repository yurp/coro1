// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

// A simple echo server using coro1 library and select backend
// Listens on port 12345, accepts incoming connections, and echoes back any received data prefixed with "ECHO: "
// Usage:
// In terminal, run this program and connect using `nc localhost 12345`
// Type messages and see them echoed back

#include "socket_helpers.hpp"

#include <co1/awaiters.hpp>
#include <co1/backend/select.hpp>
#include <co1/scheduler.hpp>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

using select_scheduler = co1::scheduler<co1::backend::select>;

class client_socket
{
public:
    explicit client_socket(co1::fd_t socket_fd) : m_socket_fd(socket_fd) { }

    client_socket(const client_socket& ) = delete;
    client_socket& operator=(const client_socket& other) = delete;

    client_socket(client_socket&& other) noexcept
        : m_socket_fd(other.m_socket_fd)
    {
        other.m_socket_fd = -1;
    }

    client_socket& operator=(client_socket&& other) noexcept
    {
        if (this != &other)
        {
            m_socket_fd = other.m_socket_fd;
            other.m_socket_fd = -1;
        }
        return *this;
    }

    ~client_socket()
    {
        if (m_socket_fd >= 0)
            ::close(m_socket_fd);
    }

    operator bool() const noexcept { return m_socket_fd >= 0; }

    co1::task<int> read_some(char* buffer, size_t length)
    {
        co1::io_op read_op { co1::io_type::read, m_socket_fd };
        co_await read_op;
        co_return recv_data(m_socket_fd, buffer, length);
    }

    co1::task<int> write_some(const char* buffer, size_t length)
    {
        co1::io_op write_op { co1::io_type::write, m_socket_fd };
        co_await write_op;
        co_return send_data(m_socket_fd, buffer, length);
    }

private:
    co1::fd_t m_socket_fd;
};

class server_socket
{
public:
    explicit server_socket(int port)
        : m_socket_fd(-1)
    {
        int s = create_socket();
        bind_socket(s, port);
        listen_socket(s);
        m_socket_fd = s;
    }

    co1::task<client_socket> accept()
    {
        co1::io_op accept_op { co1::io_type::read, m_socket_fd };
        co_await accept_op;
        co1::fd_t client_s = accept_connection(m_socket_fd);
        co_return client_socket(client_s);
    }

    ~server_socket()
    {
        if (m_socket_fd >= 0)
            ::close(m_socket_fd);
    }

private:
    co1::fd_t m_socket_fd;
};

co1::task<void> process_client(client_socket client)
{
    std::cout << "starting client handler" << std::endl;
    char buffer[1024];
    ::strcpy(buffer, "ECHO: ");
    int offs = sizeof("ECHO: ") - 1;
    while (true)
    {
        int n = co_await client.read_some(buffer + offs, sizeof(buffer) - offs);
        if (n < 0)
            continue;
        else if (n == 0)
            break;
        std::cout << "received " << n << " bytes from socket" << std::endl;

        n += offs; // account for "ECHO: "
        int sent = co_await client.write_some(buffer, n);
        if (sent < 0)
            continue;
        std::cout << "sent " << sent << " bytes to socket" << std::endl;
    }
    std::cout << "client disconnected, socket" << std::endl;
}

co1::task<void> async_main(select_scheduler& scheduler)
{
    server_socket server(12345);
    std::vector<co1::task<void>> client_handlers;
    for (int i = 0; i < 5; ++i)
    {
        std::cout << "waiting for incoming connections on port 12345..." << std::endl;
        auto client = co_await server.accept();
        if (!client)
            continue;
        std::cout << "accepted connection" << std::endl;
        scheduler.spawn(process_client(std::move(client)));
    }
}

int main()
{
    select_scheduler scheduler;
    scheduler.start(async_main(scheduler));
    return 0;
}
