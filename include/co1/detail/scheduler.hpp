// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/detail/timer_queue.hpp>

#include <coroutine>
#include <functional>

namespace co1::detail
{

template <typename Queue, typename In, typename Out>
void add_to_queue(void* obj, const In& input, Out& output, coro_ctl ctl)
{
    auto& io_queue = *static_cast<Queue*>(obj);
    io_queue.add(input, output, std::move(ctl));
}

struct io_queue_wrapper
{
    using add_t = void(*)(void*, const io_wait&, std::error_code&, coro_ctl);

    void* m_object;
    add_t m_add;

    template <typename IoQueue>
    static io_queue_wrapper make(IoQueue& queue)
    {
        return io_queue_wrapper
        {
            .m_object = &queue,
            .m_add = &add_to_queue<IoQueue, io_wait, std::error_code>,
        };
    }

    // NOLINTNEXTLINE(readability-make-member-function-const)
    void add(const io_wait& iow, std::error_code& error_code, coro_ctl ctl)
    {
        m_add(m_object, iow, error_code, std::move(ctl));
    }
};

struct scheduler
{
    explicit scheduler(io_queue_wrapper queue)
        : m_io_queue(queue)
        , m_timer_queue {}
    {
    }

    io_queue_wrapper m_io_queue;
    timer_queue m_timer_queue;
    coro_queue_t m_ready_coros;
    coro_queue_t m_finalized_coros;
};

} // namespace co1::detail
