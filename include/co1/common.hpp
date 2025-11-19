// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#define CO1_ENABLE_TRACE

#ifdef CO1_ENABLE_TRACE
#include <iostream>
#include <chrono>
#define TRACE(msg) (std::cerr << "[" << \
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() \
    << " " << __FILE_NAME__ << ":" << __LINE__ << "] " << msg << std::endl)
#else
#define TRACE(msg) ((void)0)
#endif

#include <coroutine>
#include <chrono>
#include <queue>


namespace co1
{

using fd_t = int;

using clock_t = std::chrono::steady_clock;
using time_point_t = clock_t::time_point;

enum class io_type : char
{
    read = 1,
    write = 2,
};

struct io_op
{
    io_type type;
    fd_t fd;
};

class sleep
{
public:
    sleep(std::chrono::milliseconds duration) : m_until(clock_t::now() + duration) { }
    sleep(clock_t::time_point until) : m_until(until) { }
    time_point_t until() const { return m_until; }

private:
    time_point_t m_until;
};

namespace detail
{

using coro_queue_t = std::queue<std::coroutine_handle<>>;
using ready_sink_t = coro_queue_t;

} // namespace detail

} // namespace co1
