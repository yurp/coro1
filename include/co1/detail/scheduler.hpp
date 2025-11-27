// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/detail/timer_queue.hpp>

#include <coroutine>
#include <functional>

namespace co1::detail
{

struct io_queue_wrapper
{
    template <typename IoQueue>
    io_queue_wrapper(IoQueue& queue)
        : add([&queue](io_wait iow, std::coroutine_handle<> coro) { queue.add(iow, coro); }) { }

    std::function<void(io_wait, std::coroutine_handle<>)> add;
};

struct scheduler
{
    explicit scheduler(io_queue_wrapper queue)
        : m_io_queue(std::move(queue))
        , m_timer_queue {}
    {
    }

    io_queue_wrapper m_io_queue;
    timer_queue m_timer_queue;
    coro_queue_t m_ready_coros;
    coro_queue_t m_finalized_coros;
};

} // namespace co1::detail
