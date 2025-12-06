// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>

#include <coroutine>
#include <exception>

namespace co1::detail
{

template <typename Queues>
struct promise_base
{
    struct final_awaiter
    {
        [[nodiscard]] bool await_ready() const noexcept { return false; }

        template <typename T>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<T> coro) noexcept
        {
            auto& promise = coro.promise();
            if (promise.m_parent)
            {
                TRACE("Resuming parent coroutine");
                return promise.m_parent;
            }

            TRACE("Scheduling finalized coroutine");
            auto ctl = std::move(promise.m_ctl);
            promise.m_ctl = nullptr;
            promise.m_queues->m_finalized_coros.push(std::move(ctl));
            return std::noop_coroutine();
        }

        void await_resume() noexcept { }
    };

    promise_base(const promise_base& ) = delete;
    promise_base& operator=(const promise_base& ) = delete;
    promise_base(promise_base&& ) = delete;
    promise_base& operator=(promise_base&& ) = delete;

    std::suspend_always initial_suspend() noexcept { return {}; }
    final_awaiter final_suspend() noexcept { return {}; }
    void unhandled_exception() { m_exception = std::current_exception(); }

    Queues* m_queues = nullptr;
    std::coroutine_handle<> m_parent = nullptr;
    coro_ctl m_ctl = nullptr;
    std::exception_ptr m_exception = nullptr;

protected:
    promise_base() = default;
    ~promise_base()
    {
        TRACE("Destroying promise");
        if (m_exception)
        {
            TRACE("Promise being destroyed has unhandled exception");
        }
    }
};

template <typename Queues, typename T>
struct promise : promise_base<Queues>
{
    using handle_t = std::coroutine_handle<promise>;

    auto get_return_object() { return handle_t::from_promise(*this); }

    template <typename U>
    void return_value(U&& val) noexcept { m_value = std::forward<U>(val); }

    std::optional<T> m_value;
};

template <typename Queues>
struct promise<Queues, void> : promise_base<Queues>
{
public:
    using handle_t = std::coroutine_handle<promise>;

    auto get_return_object() { return handle_t::from_promise(*this); }
    void return_void() noexcept {}
};

} // namespace co1::detail
