// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/detail/promise.hpp>

#include <utility>

namespace co1
{

template <typename Scheduler, typename T = void>
class [[nodiscard]] basic_task
{
public:
    using promise_type = detail::promise<Scheduler, T>;
    using handle_t = std::coroutine_handle<promise_type>;

    basic_task(handle_t handle) : m_handle(handle) { }

    basic_task(const basic_task& ) = delete;
    basic_task& operator=(const basic_task& ) = delete;

    basic_task(basic_task&& other) noexcept
        : m_handle(std::exchange(other.m_handle, nullptr))
    {
    }

    basic_task& operator=(basic_task&& other) noexcept
    {
        if (this != &other)
        {
            if (m_handle)
            {
                m_handle.destroy();
            }
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }

    ~basic_task()
    {
        if (m_handle)
        {
            m_handle.destroy();
        }
    }

    handle_t release() { return std::exchange(m_handle, nullptr); }

private:
    handle_t m_handle;
};

template <typename Scheduler, typename T>
class basic_task_handle
{
public:
    explicit basic_task_handle(detail::coro_ctl ctl)
        : m_coro_ctl(std::move(ctl))
    {
    }

    T get()
    {
        if (m_coro_ctl && m_coro_ctl->m_root_coro && m_coro_ctl->m_root_coro.done())
        {
            auto* addr = m_coro_ctl->m_root_coro.address();
            auto typed_handle = std::coroutine_handle<detail::promise<Scheduler, T>>::from_address(addr);
            auto& value = typed_handle.promise().m_value;
            if (value.has_value())
            {
                auto ret = std::move(*value);
                m_coro_ctl = nullptr;
                return std::move(ret);
            }
        }
        throw std::runtime_error("Task result is not ready");
    }

private:
    detail::coro_ctl m_coro_ctl;
};

template <typename Scheduler>
class basic_task_handle<Scheduler, void>
{
public:
    explicit basic_task_handle(detail::coro_ctl ctl)
        : m_coro_ctl(std::move(ctl))
    {
    }

    void get()
    {
        if (m_coro_ctl && m_coro_ctl->m_root_coro && m_coro_ctl->m_root_coro.done())
        {
            m_coro_ctl = nullptr;
            return;
        }
        throw std::runtime_error("Task result is not ready");
    }

private:
    detail::coro_ctl m_coro_ctl;
};

} // namespace co1
