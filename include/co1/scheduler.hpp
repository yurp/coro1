// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/event_queues.hpp>

#include <functional>
#include <queue>
#include <thread>

namespace co1
{

template <unique_event_queues... Qs>
class basic_scheduler
{
public:
    using pushable_context_t = pushable_context<Qs...>;

    basic_scheduler() = default;

    pushable_context_t& get_pushable_context() { return m_push_context; }

    void spawn(detail::coro_ctl coro)
    {
        TRACE("Spawning new coroutine into scheduler");
        m_ready_coros.push(std::move(coro));
    }

    bool step()
    {
        TRACE("Scheduler step");
        poll_context poll_ctx;
        poll_ctx.m_now = clock_t::now();
        if (m_push_context.m_finalized_coro != nullptr)
        {
            TRACE("Moving finalized coroutine to poll context");
            poll_ctx.m_finalized_coro = m_push_context.m_finalized_coro;
            m_push_context.m_finalized_coro = nullptr;
        }

        if (m_ready_coros.empty() && event_queues_empty(m_push_context.m_queues))
        {
            return false;
        }

        auto next_wakeup = poll_generic_event_queues(m_ready_coros, poll_ctx, m_push_context.m_queues);
        auto not_later_than = m_ready_coros.empty() ? next_wakeup : clock_t::time_point::min();
        if constexpr (has_blocking_event_queue_v<Qs...>)
        {
            get_io_event_queue(m_push_context.m_queues).poll(m_ready_coros, poll_ctx, not_later_than);
        }
        else
        {
            std::this_thread::sleep_until(not_later_than);
        }

        if (!m_ready_coros.empty())
        {
            auto coro_to_resume = m_ready_coros.front();
            m_ready_coros.pop();
            coro_to_resume->m_active_coro.resume();
        }
        else
        {
            TRACE("No ready coroutines to resume after polling");
        }

        return true;
    }

private:
    pushable_context_t m_push_context;
    detail::coro_queue_t m_ready_coros;
};

} // namespace co1
