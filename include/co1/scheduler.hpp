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
    T start(task<T>&& task_to_start)
    {
        auto tsk = spawn(std::move(task_to_start));
        run();
        return tsk.get();
    }

    template <typename T>
    task_handle<T> spawn(task<T>&& task_to_spawn)
    {
        auto moved_task = std::move(task_to_spawn);
        auto coro_handle = moved_task.release();
        auto ctl = std::make_shared<detail::control_block>(coro_handle);
        coro_handle.promise().m_scheduler = &m_impl;
        coro_handle.promise().m_ctl = ctl;

        m_impl.m_ready_coros.push(ctl);

        return task_handle<T> { std::move(ctl) };
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
                m_impl.m_finalized_coros.pop();
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
            coro_to_resume->m_active_coro.resume();
        }
        TRACE("Ending scheduler loop");
    }

private:
    IoQueue m_io_queue; ///< customization point for IO
    detail::scheduler m_impl;
};

} // namespace co1
