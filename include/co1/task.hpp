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

} // namespace co1
