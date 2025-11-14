// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#define CO1_ENABLE_TRACE

#ifdef CO1_ENABLE_TRACE
#include <iostream>
#define TRACE(msg) (std::cerr << msg << std::endl)
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

class ready_sink
{
public:
    ready_sink(std::queue<std::coroutine_handle<>>* ready_coros = nullptr) : m_ready_coros(ready_coros) { }
    void push(std::coroutine_handle<> coro) { m_ready_coros->push(coro); }

private:
    std::queue<std::coroutine_handle<>>* m_ready_coros;
};

} // namespace co1
