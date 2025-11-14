// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/promise.hpp>

namespace co1
{

template <typename T = void>
class task
{
public:
    using promise_type = promise<T>;
    using handle_t = std::coroutine_handle<promise_type>;

    task(handle_t h) : m_handle(h) { }

    ~task()
    {
        if (m_handle)
            m_handle.destroy();
    }

    // Initialize after initial_suspend
    void init(promise_context ctx)
    {
        // TODO: check if already initialized, maybe move the logic to promise?
        ctx.set_payload(init_payload{});
        ctx.set_handle(m_handle);
        m_handle.promise().init(std::move(ctx));
    }

    T get()
    {
        if constexpr (std::is_void_v<T>)
            return;
        else
            // TODO: handle empty optional
            return std::move(m_handle.promise().get_value().value());
    }

private:
    handle_t m_handle;
};

} // namespace co1
