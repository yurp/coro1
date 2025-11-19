// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/detail/promise.hpp>

namespace co1
{

struct sleep_awaitable
{
public:
    sleep_awaitable(sleep s) : m_sleep(s)  { }

    bool await_ready() const noexcept { return false; }

    template <typename T>
    void await_suspend(std::coroutine_handle<detail::promise<T>> coro) noexcept
    {
        coro.promise().m_scheduler->m_timer_queue.add(m_sleep.until(), coro);
    }
    void await_resume() noexcept { }

private:
    sleep m_sleep;
};

sleep_awaitable operator co_await(sleep s)
{
    return sleep_awaitable(s);
}

struct io_awaitable
{
public:
    io_awaitable(io_op op) : m_io_op(std::move(op))  { }

    bool await_ready() const noexcept { return false; }

    template <typename T>
    void await_suspend(std::coroutine_handle<detail::promise<T>> coro) noexcept
    {
        coro.promise().m_scheduler->m_poller.add(m_io_op, coro);
    }
    void await_resume() noexcept { }

private:
    io_op m_io_op;
};

io_awaitable operator co_await(io_op op)
{
    return io_awaitable(std::move(op));
}

} // namespace co1
