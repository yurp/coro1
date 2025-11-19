// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

// A simple echo server using coro1 library and select backend
// Listens on port 12345, accepts incoming connections, and echoes back any received data prefixed with "ECHO: "
// Usage:
// In terminal, run this program and connect using `nc localhost 12345`
// Type messages and see them echoed back

#include "socket_helpers.hpp"

#include <co1/core_awaitable.hpp>
#include <co1/backend/select.hpp>
#include <co1/scheduler.hpp>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

using select_scheduler = co1::scheduler<co1::backend::select>;

co1::task<void> process_client(int client_s)
{
    char buffer[1024];
    ::strcpy(buffer, "ECHO: ");
    int offs = sizeof("ECHO: ") - 1;
    while (true)
    {
        co1::io_op read_op { co1::io_type::read, client_s };
        co_await read_op;

        ssize_t n = recv_data(client_s, buffer + offs, sizeof(buffer) - offs);
        if (n < 0)
            continue;
        else if (n == 0)
            break;
        std::cout << "received " << n << " bytes from socket " << client_s << std::endl;

        co1::io_op write_op { co1::io_type::write, client_s };
        co_await write_op;

        n += offs; // account for "ECHO: "
        ssize_t sent = send_data(client_s, buffer, n);
        if (sent < 0)
            continue;
        std::cout << "sent " << sent << " bytes to socket " << client_s << std::endl;
    }
    std::cout << "client disconnected, socket: " << client_s << std::endl;
    ::close(client_s);
}

co1::task<int> async_main(auto& scheduler)
{
    int s = create_socket();
    bind_socket(s, 12345);
    listen_socket(s);
    while (true)
    {
        std::cout << "waiting for incoming connections on port 12345..." << std::endl;

        co1::io_op accept_op { co1::io_type::read, s };
        co_await accept_op;

        int client_s = accept_connection(s);
        if (client_s < 0)
            continue;
        std::cout << "accepted connection, socket: " << client_s << std::endl;

        scheduler.spawn(process_client(client_s));
    }
    co_return 0;
}

int main()
{
    select_scheduler scheduler;
    scheduler.start(async_main(scheduler));
    return 0;
}
