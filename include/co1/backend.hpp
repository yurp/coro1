
#pragma once

#include <chrono>
#include <coroutine>
#include <concepts>
#include <vector>

namespace co1
{

enum class event_type : char
{
    fd_read = 1,
    fd_write = 2,
};

struct event
{
    event_type type;
    int fd;

    event(int fd = -1, event_type type = event_type::fd_read) : fd(fd), type(type) {}
    bool operator==(const event& other) const = default;
};

} // namespace co1
