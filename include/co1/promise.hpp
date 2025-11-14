// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/core_awaitable.hpp>

#include <coroutine>

namespace co1
{

class promise_base
{
public:
    std::suspend_always initial_suspend() noexcept
    {
        // if context already set, maybe set it and return core_awaitable?
        return {};
    }

    core_awaitable final_suspend() noexcept
    {
        TRACE("Final suspend called");
        m_context.set_payload(finalize_payload{});
        return core_awaitable { m_context };
    }

    void unhandled_exception()
    {
        std::terminate();
    }

    template <typename Rep, typename Period>
    auto await_transform(std::chrono::duration<Rep, Period> duration)
    {
        m_context.set_payload(timer_payload { clock_t::now() + duration });
        return core_awaitable { m_context };
    }

    auto await_transform(io_op op)
    {
        m_context.set_payload(io_payload { op });
        return core_awaitable { m_context };
    }

    void init(promise_context ctx)
    {
        m_context = std::move(ctx);
    }

protected:
    promise_base() = default;
    ~promise_base() { TRACE("Destroying promise"); }

    promise_context m_context;
};

template <typename T>
class promise : public promise_base
{
public:
    using handle_t = std::coroutine_handle<promise>;

    auto get_return_object()
    {
        return handle_t::from_promise(*this);
    }

    void return_value(T&& v) noexcept
    {
        TRACE("Returning value from coroutine: " << v);
        m_value = std::forward<T>(v);
    }

    std::optional<T> get_value() noexcept
    {
        return std::move(m_value);
    }

private:
    std::optional<T> m_value;
};

template <>
class promise<void> : public promise_base
{
public:
    using handle_t = std::coroutine_handle<promise>;

    auto get_return_object() { return handle_t::from_promise(*this); }
    void return_void() noexcept {}
};

} // namespace co1
