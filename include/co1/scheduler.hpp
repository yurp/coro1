// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/event_queues.hpp>
#include <co1/task.hpp>

#include <functional>
#include <queue>
#include <thread>

namespace co1
{

template <unique_event_queues... Qs>
class basic_scheduler
{
public:
    template <typename T>
    using task_t = basic_task<T, Qs...>;

    template <typename T>
    using task_handle_t = basic_task_handle<T, Qs...>;

    basic_scheduler() = default;

    template <typename T>
    T start(task_t<T>&& task_to_start)
    {
        auto tsk = spawn(std::move(task_to_start));
        run();
        return tsk.get();
    }

    template <typename T>
    task_handle_t<T> spawn(task_t<T>&& task_to_spawn)
    {
        auto moved_task = std::move(task_to_spawn);
        auto coro_handle = moved_task.release();
        auto ctl = std::make_shared<detail::control_block>(coro_handle);
        coro_handle.promise().m_context = &m_push_context;
        coro_handle.promise().m_ctl = ctl;

        m_ready_coros.push(ctl);

        return task_handle_t<T> { std::move(ctl) };
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

    void run()
    {
        TRACE("Starting scheduler loop");
        bool has_work = true;
        while (has_work)
        {
            has_work = step();
        }
        TRACE("Ending scheduler loop");
    }

private:
    pushable_context<Qs...> m_push_context;
    detail::coro_queue_t m_ready_coros;
};

} // namespace co1
