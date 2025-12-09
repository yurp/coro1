// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

// A simple echo server using coro1 library and select backend
// Listens on port 12345, accepts incoming connections, and echoes back any received data prefixed with "ECHO: "
// Usage:
// In terminal, run this program and connect using `nc localhost 12345`
// Type messages and see them echoed back

#include "socket_helpers.hpp"

#include <co1/async_main.hpp>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <iostream>

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
        {
            ::close(m_socket_fd);
        }
    }

    operator bool() const noexcept { return m_socket_fd >= 0; }

    co1::task<int> read_some(char* buffer, size_t length)
    {
        std::error_code ecode = co_await co1::io_wait { co1::io_type::read, m_socket_fd };
        if (ecode)
        {
            co_return -1;
        }
        co_return recv_data(m_socket_fd, buffer, length);
    }

    co1::task<int> write_some(const char* buffer, size_t length)
    {
        std::error_code ecode = co_await co1::io_wait { co1::io_type::write, m_socket_fd };
        if (ecode)
        {
            co_return -1;
        }
        co_return send_data(m_socket_fd, buffer, length);
    }

private:
    co1::fd_t m_socket_fd;
};

class server_socket
{
public:
    explicit server_socket(int port)
    {
        int socket_fd = create_socket();
        bind_socket(socket_fd, port);
        listen_socket(socket_fd);
        m_socket_fd = socket_fd;
    }

    server_socket(const server_socket& ) = delete;
    server_socket& operator=(const server_socket& other) = delete;

    server_socket(server_socket&& other) noexcept
        : m_socket_fd(other.m_socket_fd)
    {
        other.m_socket_fd = -1;
    }

    server_socket& operator=(server_socket&& other) noexcept
    {
        if (this != &other)
        {
            m_socket_fd = other.m_socket_fd;
            other.m_socket_fd = -1;
        }
        return *this;
    }

    ~server_socket()
    {
        if (m_socket_fd >= 0)
        {
            ::close(m_socket_fd);
        }
    }

    co1::task<client_socket> accept()
    {
        co1::io_wait wait_accept { co1::io_type::read, m_socket_fd };
        co_await wait_accept;
        co1::fd_t client_s = accept_connection(m_socket_fd);
        co_return client_socket(client_s);
    }


private:
    co1::fd_t m_socket_fd = -1;
};

co1::task<> process_client(client_socket client)
{
    static constexpr size_t BUFFER_SIZE = 1024;

    std::cout << "starting client handler" << std::endl;
    std::array<char, BUFFER_SIZE> buffer {};
    ::strncpy(buffer.data(), "ECHO: ", buffer.size());
    int offs = sizeof("ECHO: ") - 1;
    while (true)
    {
        int read_cnt = co_await client.read_some(buffer.data() + offs, buffer.size() - offs);
        if (read_cnt < 0)
        {
            continue;
        }
        if (read_cnt == 0)
        {
            break;
        }
        std::cout << "received " << read_cnt << " bytes from socket" << std::endl;

        read_cnt += offs; // account for "ECHO: "
        int sent_cnt = co_await client.write_some(buffer.data(), read_cnt);
        if (sent_cnt < 0)
        {
            continue;
        }
        std::cout << "sent " << sent_cnt << " bytes to socket" << std::endl;
    }
    std::cout << "client disconnected, socket" << std::endl;
}

co1::task<int> async_main(int /*argc*/, char** /*argv*/)
{
    static constexpr int PORT = 12345;
    static constexpr size_t MAX_CLIENTS = 5;

    server_socket server(PORT);
    std::vector<co1::task<void>> client_handlers;
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
    {
        std::cout << "waiting for incoming connections on port " << PORT << "..." << std::endl;
        if (auto client = co_await server.accept())
        {
            std::cout << "accepted connection" << std::endl;
            co1::local_async_context::spawn(process_client(std::move(client)));
        }
    }

    co_return 0;
}
