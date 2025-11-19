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

template <typename Poller>
class scheduler
{
public:
    scheduler()
        : m_poller()
        , m_impl(m_poller)
    {
    }

    template <typename T>
    void start(task<T>&& t)
    {
        spawn(std::move(t));
        run();
    }

    template <typename T>
    void spawn(task<T>&& t)
    {
        auto h = t.release();
        h.promise().m_scheduler = &m_impl;
        m_impl.m_ready_coros.push(h);
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
                m_impl.m_timer_queue.empty() && m_poller.empty())
                break;

            while (m_impl.m_ready_coros.empty())
            {
                TRACE("No ready coroutines, polling timers and IO events");
                auto tp = m_impl.m_timer_queue.poll(m_impl.m_ready_coros);
                auto now = clock_t::now();
                auto duration = (m_impl.m_ready_coros.empty() && tp > now) ? tp - now : clock_t::duration::zero();
                m_poller.poll(m_impl.m_ready_coros, duration);
            }

            auto coro_to_resume = m_impl.m_ready_coros.front();
            m_impl.m_ready_coros.pop();
            coro_to_resume.resume();
        }
        TRACE("Ending scheduler loop");
    }

private:
    Poller m_poller; ///< customization point for IO polling
    detail::scheduler m_impl;
};

} // namespace co1
