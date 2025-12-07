// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/detail/promise.hpp>
#include <co1/event_queues.hpp>
#include <co1/task.hpp>

namespace co1
{

template <typename In>
struct base_event_queue_awaiter
{
    base_event_queue_awaiter(const base_event_queue_awaiter& ) = delete;
    base_event_queue_awaiter& operator=(const base_event_queue_awaiter& ) = delete;
    base_event_queue_awaiter(base_event_queue_awaiter&& ) = delete;
    base_event_queue_awaiter& operator=(base_event_queue_awaiter&& ) = delete;
    ~base_event_queue_awaiter() = default;

    template <typename T>
    explicit base_event_queue_awaiter(T&& input) : m_input(std::forward<T>(input))  { }

    [[nodiscard]] bool await_ready() noexcept { return false; }

    In m_input;
};

template <typename In, typename Result = void>
    requires (std::is_void_v<Result> || std::default_initializable<Result>)
struct event_queue_awaiter : base_event_queue_awaiter<In>
{
    using base_event_queue_awaiter<In>::base_event_queue_awaiter;

    template <typename Ctx>
    void await_suspend(std::coroutine_handle<Ctx> coro) noexcept
        requires pushable_with_result<Ctx, In, Result>
    {
        coro.promise().push_to_queue(std::move(this->m_input), m_output);
    }

    Result await_resume() noexcept { return std::move(m_output); }

    Result m_output;
};

template <typename In>
struct event_queue_awaiter<In, void> : base_event_queue_awaiter<In>
{
    using base_event_queue_awaiter<In>::base_event_queue_awaiter;

    template <typename Ctx>
    void await_suspend(std::coroutine_handle<Ctx> coro) noexcept
        requires pushable<Ctx, In>
    {
        coro.promise().push_to_queue(std::move(this->m_input));
    }
    void await_resume() noexcept { }
};

event_queue_awaiter<time_point_t> operator co_await(wait time_wait)
{
    return event_queue_awaiter<time_point_t> { time_wait.until() };
}

event_queue_awaiter<io_wait, std::error_code> operator co_await(io_wait iow)
{
    return event_queue_awaiter<io_wait, std::error_code> { iow };
}

template <typename T, unique_event_queues... Qs>
class task_awaiter
{
public:
    explicit task_awaiter(basic_task<T, Qs...>::handle_t handle) : m_handle(handle) { }
    [[nodiscard]] bool await_ready() const noexcept { return false; }

    template<typename U>
    auto await_suspend(std::coroutine_handle<detail::promise<U, Qs...>> parent) noexcept
    {
        TRACE("Suspending current coroutine and resuming awaited coroutine");
        auto& awaited_promise = m_handle.promise();
        auto& parent_promise = parent.promise();
        awaited_promise.m_context = parent_promise.m_context;
        awaited_promise.m_ctl = parent_promise.m_ctl;
        awaited_promise.m_ctl->m_active_coro = m_handle;
        awaited_promise.m_parent = parent;

        return m_handle;
    }

    T await_resume()
    {
        auto& awaited_promise = m_handle.promise();
        awaited_promise.m_ctl->m_active_coro = awaited_promise.m_parent;

        if (awaited_promise.m_exception)
        {
            TRACE("Rethrowing exception from awaited task");
            auto exception = awaited_promise.m_exception;
            awaited_promise.m_exception = nullptr;
            m_handle.destroy();
            std::rethrow_exception(exception);
        }

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
    basic_task<T, Qs...>::handle_t m_handle;
};

template <typename T, unique_event_queues... Qs>
task_awaiter<T, Qs...> operator co_await(basic_task<T, Qs...>&& awaited_task) noexcept
{
    auto moved_task = std::move(awaited_task);
    return task_awaiter<T, Qs...> { moved_task.release() };
}

} // namespace co1
