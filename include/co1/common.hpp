// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#ifdef CO1_ENABLE_TRACE
#include <iostream>
#include <chrono>
// NOLINTBEGIN(cppcoreguidelines-macro-usage, bugprone-macro-parentheses)
#define TRACE(msg) (std::cerr << "[" << \
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() \
    << " " << __FILE_NAME__ << ":" << __LINE__ << "] " << msg << std::endl)
// NOLINTEND(cppcoreguidelines-macro-usage, bugprone-macro-parentheses)
#else
#define TRACE(msg) ((void)0)
#endif

#include <coroutine>
#include <chrono>
#include <memory>
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

struct io_wait
{
    io_type type;
    fd_t fd;
};

class wait
{
public:
    wait(std::chrono::milliseconds duration) : m_until(clock_t::now() + duration) { }
    wait(clock_t::time_point until) : m_until(until) { }
    [[nodiscard]] time_point_t until() const { return m_until; }

private:
    time_point_t m_until;
};

namespace detail
{

struct control_block
{
    std::coroutine_handle<> m_root_coro = nullptr;
    std::coroutine_handle<> m_active_coro = nullptr;

    explicit control_block(std::coroutine_handle<> coro)
        : m_root_coro(coro)
        , m_active_coro(coro)
    {
    }

    control_block() = default;
    control_block(const control_block& ) = delete;
    control_block& operator=(const control_block& ) = delete;
    control_block(control_block&& ) = delete;
    control_block& operator=(control_block&& ) = delete;

    ~control_block()
    {
        TRACE("Destroying control block");
        if (m_root_coro)
        {
            m_root_coro.destroy();
        }
    }
};

using coro_ctl = std::shared_ptr<control_block>;

using coro_queue_t = std::queue<coro_ctl>;
using ready_sink_t = coro_queue_t;

} // namespace detail

} // namespace co1
