// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/task.hpp>
#include <co1/detail/scheduler.hpp>

#include <functional>
#include <queue>

namespace co1
{

template <typename IoQueue>
class scheduler
{
public:
    scheduler()
        : m_io_queue()
        , m_impl(detail::io_queue_wrapper::make(m_io_queue))
    {
    }

    template <typename T>
    void start(task<T>&& task_to_start)
    {
        spawn(std::move(task_to_start));
        run();
    }

    template <typename T>
    void spawn(task<T>&& task_to_spawn)
    {
        auto moved_task = std::move(task_to_spawn);
        auto coro_handle = moved_task.release();
        coro_handle.promise().m_scheduler = &m_impl;
        m_impl.m_ready_coros.push(coro_handle);
    }

    void run()
    {
        TRACE("Starting scheduler loop");
        while (true)
        {
            TRACE("Scheduler loop iteration");

            while (!m_impl.m_finalized_coros.empty())
            {
                TRACE("Destroying finalized coroutine");
                auto coro_to_destroy = m_impl.m_finalized_coros.front();
                m_impl.m_finalized_coros.pop();
                coro_to_destroy.destroy();
            }

            if (m_impl.m_ready_coros.empty() && m_impl.m_finalized_coros.empty() &&
                m_impl.m_timer_queue.empty() && m_io_queue.empty())
            {
                break;
            }

            while (m_impl.m_ready_coros.empty())
            {
                TRACE("No ready coroutines, polling timers and IO events");
                auto time_point = m_impl.m_timer_queue.poll(m_impl.m_ready_coros);
                auto now = clock_t::now();
                auto duration = (m_impl.m_ready_coros.empty() && time_point > now) ?
                                    time_point - now :
                                    clock_t::duration::zero();
                m_io_queue.poll(m_impl.m_ready_coros, duration);
            }

            auto coro_to_resume = m_impl.m_ready_coros.front();
            m_impl.m_ready_coros.pop();
            coro_to_resume.resume();
        }
        TRACE("Ending scheduler loop");
    }

private:
    IoQueue m_io_queue; ///< customization point for IO
    detail::scheduler m_impl;
};

} // namespace co1
