// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/backend.hpp>
#include <co1/event_queue.hpp>
#include <co1/task.hpp>
#include <co1/utils.hpp>

namespace co1
{

template <typename Poller>
class scheduler
{
public:
    template <typename F>
    auto start(F&& f)
    {
        task t = std::forward<F>(f)();
        t.set_events_queue(&m_events);
        t.resume();
        run();
        return t.get();
    }

    void run()
    {
        while (!m_events.empty())
        {
            TRACE("Scheduler loop iteration");
            auto now = std::chrono::steady_clock::now();
            m_events.update_ready_timers(now);
            while (auto coro = m_events.pop_ready_timer())
            {
                TRACE("Resuming task");
                coro.resume();
            }

            if (!m_events.empty())
            {
                auto timeout = m_events.time_until_next_timer(now);
                TRACE("Polling with timeout: " << timeout.count() << " µs");
                m_poller.poll(timeout);
                TRACE("Polled events");
            }
        }
    }

private:
    event_queue m_events;
    Poller m_poller;
};

} // namespace co1
