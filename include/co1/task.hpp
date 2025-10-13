// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/event_queue.hpp>
#include <co1/utils.hpp>

#include <chrono>
#include <coroutine>
#include <iostream>
#include <optional>

namespace co1
{

template <typename Rep, typename Period>
class sleep_awaitable
{
public:
    sleep_awaitable(std::chrono::duration<Rep, Period> duration, event_queue* events)
        : m_duration(duration)
        , m_events(events)
    {
    }

    bool await_ready() const noexcept
    {
        return false;
    }

    void await_suspend(std::coroutine_handle<> h) noexcept
    {
        m_events->add_timer(m_duration, h);
    }

    void await_resume() noexcept
    {
    }

private:
    std::chrono::duration<Rep, Period> m_duration;
    event_queue* m_events;
};

struct promise_base
{
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept
    {
        TRACE("Final suspend called");
        return {};
    }
    void unhandled_exception() { std::terminate(); }

    template <typename Rep, typename Period>
    auto await_transform(std::chrono::duration<Rep, Period> ms)
    {
        return sleep_awaitable{std::chrono::duration_cast<std::chrono::milliseconds>(ms), m_events};
    }

    event_queue* m_events = nullptr;
};

template <typename T>
struct promise : promise_base
{
    using handle_t = std::coroutine_handle<promise>;

    auto get_return_object() { return handle_t::from_promise(*this); }
    void return_value(T&& v) noexcept
    {
        TRACE("Returning value from coroutine: " << v);
        m_value = std::forward<T>(v);
    }


    std::optional<T> m_value;
};

template <>
struct promise<void> : promise_base
{
    using handle_t = std::coroutine_handle<promise>;

    auto get_return_object() { return handle_t::from_promise(*this); }
    void return_void() noexcept {}
};

template <typename T = void>
struct task
{
    using promise_type = promise<T>;

    using handle_t = std::coroutine_handle<promise_type>;
    handle_t m_handle;

    void set_events_queue(event_queue* events)
    {
        m_handle.promise().m_events = events;
    }

    task(handle_t h) : m_handle(h) { }
    ~task() { if (m_handle) m_handle.destroy(); }

    bool resume()
    {
        if (!m_handle.done())
            m_handle.resume();
        return !m_handle.done();
    }

    bool done() const
    {
        return m_handle.done();
    }

    T get()
    {
        if constexpr (std::is_void_v<T>)
            return;
        else
            return m_handle.promise().m_value.value();
    }
};

} // namespace co1
