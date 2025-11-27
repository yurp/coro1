// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/detail/promise.hpp>
#include <co1/task.hpp>

namespace co1
{

struct sleep_awaiter
{
public:
    sleep_awaiter(sleep slp) : m_sleep(slp)  { }

    [[nodiscard]] bool await_ready() noexcept { return false; }

    template <typename T>
    void await_suspend(std::coroutine_handle<detail::promise<T>> coro) noexcept
    {
        coro.promise().m_scheduler->m_timer_queue.add(m_sleep.until(), coro);
    }
    void await_resume() noexcept { }

private:
    sleep m_sleep;
};

sleep_awaiter operator co_await(sleep slp)
{
    return { slp };
}

struct io_awaiter
{
public:
    io_awaiter(io_op iop) : m_io_op(iop)  { }

    [[nodiscard]] bool await_ready() const noexcept { return false; }

    template <typename T>
    void await_suspend(std::coroutine_handle<detail::promise<T>> coro) noexcept
    {
        coro.promise().m_scheduler->m_io_queue.add(m_io_op, coro);
    }
    void await_resume() noexcept { }

private:
    io_op m_io_op;
};

io_awaiter operator co_await(io_op iop)
{
    return { iop };
}

template <typename T>
class task_awaiter
{
public:
    explicit task_awaiter(task<T>::handle_t handle) : m_handle(handle) { }

    [[nodiscard]] bool await_ready() const noexcept { return false; }

    template<typename U>
    auto await_suspend(std::coroutine_handle<detail::promise<U>> parent) noexcept
    {
        TRACE("Suspending current coroutine and resuming awaited coroutine");
        m_handle.promise().m_scheduler = parent.promise().m_scheduler;
        m_handle.promise().m_parent = parent;

        return m_handle;
    }

    T await_resume()
    {
        TRACE("Resuming after await, updaiting call stack");
        if constexpr (std::is_void_v<T>)
        {
            m_handle.destroy();
            return;
        }
        else
        {
            auto& promise_val = m_handle.promise().m_value;
            if (promise_val.has_value())
            {
                auto value = std::move(*promise_val);
                m_handle.destroy();
                return value;
            }
            m_handle.destroy();
            throw std::runtime_error("Attempt to co_await a task with no return value");
        }
    }

private:
    task<T>::handle_t m_handle;
};

template <typename T>
task_awaiter<T> operator co_await(task<T>&& awaited_task) noexcept
{
    auto moved_task = std::move(awaited_task);
    return task_awaiter<T> { moved_task.release() };
}


} // namespace co1
