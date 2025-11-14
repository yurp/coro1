// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/context.hpp>

#include <coroutine>

namespace co1
{

class core_awaitable
{
public:
    core_awaitable(promise_context ctx) : m_context(ctx)  { }

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> coro) noexcept { m_context.set_handle(coro); }
    void await_resume() noexcept { }

private:
    promise_context m_context;
};

} // namespace co1
