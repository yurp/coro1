// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <system_error>

inline std::system_error make_errno(const std::string& msg)
{
    return std::system_error(errno, std::generic_category(), msg);
}

inline int create_socket()
{
    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
        throw make_errno("socket");

    int value = 1;
    socklen_t len = sizeof(value);
    // Enable sending of keep-alive messages on connection-oriented sockets.  Expects an integer boolean flag.
    if (::setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*) &value, len) < 0)
        throw make_errno("setsockopt");

    auto flags = ::fcntl(s, F_GETFL, 0);
    if (flags < 0)
        throw make_errno("fcntl");

    if (::fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0)
        throw make_errno("fcntl");

    return s;
}

inline void bind_socket(int s, uint16_t port)
{
    ::sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = ::htons(port);

    int optval = 1;
    socklen_t optlen = sizeof(optval);
    if (::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*) &optval, optlen) < 0)
        throw make_errno("setsockopt");

    if (::bind(s, (struct sockaddr*) &addr, sizeof(addr)) < 0)
        throw make_errno("bind");
}

inline void listen_socket(int s)
{
    if (::listen(s, SOMAXCONN) < 0)
        throw make_errno("listen");
}

inline int accept_connection(int s)
{
    ::sockaddr_in client_addr {};
    socklen_t client_len = sizeof(client_addr);
    int client_s = ::accept(s, (struct sockaddr*) &client_addr, &client_len);
    if (client_s < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            std::cerr << "[socket helpers] accept() would block, retrying..." << std::endl;
        else
            throw make_errno("accept");
    }
    return client_s;
}

inline int recv_data(int s, char* buffer, size_t length)
{
    ssize_t n = ::recv(s, buffer, length, 0);
    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            std::cerr << "[socket helpers] recv() would block, retrying..." << std::endl;
            return -1;
        }
        throw make_errno("recv");
    }
    return n;
}

inline int send_data(int s, const char* buffer, size_t length)
{
    ssize_t n = ::send(s, buffer, length, 0);
    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            std::cerr << "[socket helpers] send() would block, retrying..." << std::endl;
            return -1;
        }
        throw make_errno("send");
    }
    return n;
}
