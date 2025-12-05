// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/task.hpp>

#include <functional>
#include <queue>

namespace co1
{

template <typename IoQueue, typename TimerQueue>
class basic_scheduler
{
public:
    struct queues
    {
        IoQueue m_io_queue;
        TimerQueue m_timer_queue;
        detail::coro_queue_t m_ready_coros;
        detail::coro_queue_t m_finalized_coros;
    };

    template <typename T>
    using task_t = basic_task<queues, T>;

    template <typename T>
    using task_handle_t = basic_task_handle<queues, T>;

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
        coro_handle.promise().m_queues = &m_queues;
        coro_handle.promise().m_ctl = ctl;

        m_queues.m_ready_coros.push(ctl);

        return task_handle_t<T> { std::move(ctl) };
    }

    void run()
    {
        TRACE("Starting scheduler loop");
        while (true)
        {
            TRACE("Scheduler loop iteration");

            while (!m_queues.m_finalized_coros.empty())
            {
                TRACE("Destroying finalized coroutine");
                m_queues.m_finalized_coros.pop();
            }

            if (m_queues.m_ready_coros.empty() && m_queues.m_finalized_coros.empty() &&
                m_queues.m_timer_queue.empty() && m_queues.m_io_queue.empty())
            {
                break;
            }

            while (m_queues.m_ready_coros.empty())
            {
                TRACE("No ready coroutines, polling timers and IO events");
                auto time_point = m_queues.m_timer_queue.poll(m_queues.m_ready_coros);
                auto now = clock_t::now();
                auto duration = (m_queues.m_ready_coros.empty() && time_point > now) ?
                                    time_point - now :
                                    clock_t::duration::zero();
                m_queues.m_io_queue.poll(m_queues.m_ready_coros, duration);
            }

            auto coro_to_resume = m_queues.m_ready_coros.front();
            m_queues.m_ready_coros.pop();
            coro_to_resume->m_active_coro.resume();
        }
        TRACE("Ending scheduler loop");
    }

private:
    queues m_queues;
};

} // namespace co1
