// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>

#include <chrono>
#include <coroutine>
#include <variant>

namespace co1
{

struct none_payload {};
struct init_payload {};
struct finalize_payload {};

struct timer_payload
{
    time_point_t tp;
};

struct io_payload
{
    io_op operation;
};

using promise_payload = std::variant<none_payload,
                                     init_payload,
                                     finalize_payload,
                                     timer_payload,
                                     io_payload>;

struct context
{
    enum payload_type
    {
        none = 0,
        init = 1,
        finalize = 2,
        timer = 3,
        io = 4,
    };

    promise_payload payload;
    std::coroutine_handle<> coro;
};

class promise_context
{
public:
    promise_context(context* ctx = nullptr) : m_context(ctx) { }

    void set_handle(std::coroutine_handle<> h) { m_context->coro = h; }

    void set_payload(init_payload)     { set_payload_impl(init_payload {}); }
    void set_payload(finalize_payload) { set_payload_impl(finalize_payload {}); }
    void set_payload(timer_payload tp) { set_payload_impl(tp); }
    void set_payload(io_payload io)    { set_payload_impl(io); }

private:
    void set_payload_impl(promise_payload p)
    {
        if (!m_context)
        {
            TRACE("Context is null");
            throw std::runtime_error("Context is null");
        }
        if (std::holds_alternative<none_payload>(m_context->payload) == false)
        {
            TRACE("Payload is already set");
            throw std::runtime_error("Payload is already set");
        }

        m_context->payload = std::move(p);
    }
    context* m_context;
};

} // namespace co1
