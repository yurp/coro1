// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/detail/scheduler.hpp>

#include <coroutine>

namespace co1::detail
{

struct promise_base
{
    struct final_awaiter
    {
        bool await_ready() const noexcept { return false; }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> coro) noexcept
        {
            if (m_parent)
            {
                TRACE("Resuming parent coroutine");
                return m_parent;
            }
            TRACE("Scheduling finalized coroutine");
            m_scheduler->m_finalized_coros.push(coro);
            return std::noop_coroutine();
        }
        void await_resume() noexcept { }

        detail::scheduler* m_scheduler;
        std::coroutine_handle<> m_parent;
    };

    std::suspend_always initial_suspend() noexcept { return {}; }
    final_awaiter final_suspend() noexcept { return final_awaiter { m_scheduler, m_parent }; }
    void unhandled_exception() { std::terminate(); }

    detail::scheduler* m_scheduler = nullptr;
    std::coroutine_handle<> m_parent = nullptr;

protected:
    promise_base() = default;
    ~promise_base() { TRACE("Destroying promise"); }
};

template <typename T>
struct promise : promise_base
{
    using handle_t = std::coroutine_handle<promise>;

    auto get_return_object() { return handle_t::from_promise(*this); }
    void return_value(T&& v) noexcept { m_value = std::forward<T>(v); }

    std::optional<T> m_value;
};

template <>
struct promise<void> : promise_base
{
public:
    using handle_t = std::coroutine_handle<promise>;

    auto get_return_object() { return handle_t::from_promise(*this); }
    void return_void() noexcept {}
};

} // namespace co1::detail
