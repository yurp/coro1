// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/detail/timer_queue.hpp>

#include <coroutine>
#include <functional>

namespace co1::detail
{

struct poller
{
    template <typename Poller>
    poller(Poller& p) : add([&p](io_op op, std::coroutine_handle<> coro) { p.add(std::move(op), coro); }) { }

    std::function<void(io_op, std::coroutine_handle<>)> add;
};

struct scheduler
{
    poller m_poller;
    timer_queue m_timer_queue;
    coro_queue_t m_ready_coros;
    coro_queue_t m_finalized_coros;
};

} // namespace co1::detail
