// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/detail/promise.hpp>

#include <utility>

namespace co1
{

template <typename T = void>
class [[nodiscard]] task
{
public:
    using promise_type = detail::promise<T>;
    using handle_t = std::coroutine_handle<promise_type>;

    task(handle_t handle) : m_handle(handle) { }

    task(const task& ) = delete;
    task& operator=(const task& ) = delete;

    task(task&& other) noexcept
        : m_handle(std::exchange(other.m_handle, nullptr))
    {
    }

    task& operator=(task&& other) noexcept
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

    ~task()
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

template <typename T>
class task_handle
{
public:
    explicit task_handle(detail::coro_ctl ctl)
        : m_coro_ctl(std::move(ctl))
    {
    }

    T get()
    {
        if (m_coro_ctl && m_coro_ctl->m_root_coro && m_coro_ctl->m_root_coro.done())
        {
            auto* addr = m_coro_ctl->m_root_coro.address();
            auto typed_handle = std::coroutine_handle<detail::promise<T>>::from_address(addr);
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

template <>
class task_handle<void>
{
public:
    explicit task_handle(detail::coro_ctl ctl)
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
