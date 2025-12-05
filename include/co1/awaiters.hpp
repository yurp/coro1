// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/detail/promise.hpp>
#include <co1/task.hpp>

namespace co1
{

struct time_awaiter
{
public:
    time_awaiter(wait time_wait) : m_time_wait(time_wait)  { }

    [[nodiscard]] bool await_ready() noexcept { return false; }

    template <typename Queues, typename T>
    void await_suspend(std::coroutine_handle<detail::promise<Queues, T>> coro) noexcept
    {
        auto& promise = coro.promise();
        promise.m_queues->m_timer_queue.add(m_time_wait.until(), promise.m_ctl);
    }
    void await_resume() noexcept { }

private:
    wait m_time_wait;
};

time_awaiter operator co_await(wait time_wait)
{
    return { time_wait };
}

struct io_awaiter
{
public:
    io_awaiter(io_wait iow) : m_io_wait(iow)  { }

    [[nodiscard]] bool await_ready() const noexcept { return false; }

    template <typename Queues, typename T>
    void await_suspend(std::coroutine_handle<detail::promise<Queues, T>> coro) noexcept
    {
        auto& promise = coro.promise();
        promise.m_queues->m_io_queue.add(m_io_wait, m_error_code, promise.m_ctl);
    }

    std::error_code await_resume() noexcept
    {
        return m_error_code;
    }

private:
    io_wait m_io_wait;
    std::error_code m_error_code;
};

io_awaiter operator co_await(io_wait iow)
{
    return { iow };
}

template <typename Queues, typename T>
class task_awaiter
{
public:
    explicit task_awaiter(basic_task<Queues, T>::handle_t handle) : m_handle(handle) { }
    [[nodiscard]] bool await_ready() const noexcept { return false; }

    template<typename U>
    auto await_suspend(std::coroutine_handle<detail::promise<Queues, U>> parent) noexcept
    {
        TRACE("Suspending current coroutine and resuming awaited coroutine");
        auto& awaited_promise = m_handle.promise();
        auto& parent_promise = parent.promise();
        awaited_promise.m_queues = parent_promise.m_queues;
        awaited_promise.m_ctl = parent_promise.m_ctl;
        awaited_promise.m_ctl->m_active_coro = m_handle;
        awaited_promise.m_parent = parent;

        return m_handle;
    }

    T await_resume()
    {
        auto& awaited_promise = m_handle.promise();
        awaited_promise.m_ctl->m_active_coro = awaited_promise.m_parent;

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
    basic_task<Queues, T>::handle_t m_handle;
};

template <typename Queues, typename T>
task_awaiter<Queues, T> operator co_await(basic_task<Queues, T>&& awaited_task) noexcept
{
    auto moved_task = std::move(awaited_task);
    return task_awaiter<Queues, T> { moved_task.release() };
}


} // namespace co1
