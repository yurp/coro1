// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>

#include <chrono>
#include <coroutine>
#include <queue>

namespace co1::detail
{

class timer_queue
{
public:

    struct timer
    {
        time_point_t time;
        coro_ctl ctl;

        bool operator<(const timer& other) const { return time > other.time; }
    };

    void add(time_point_t time_point, coro_ctl ctl)
    {
        using namespace std::chrono;
        m_timers.emplace(timer { time_point, std::move(ctl) });
        TRACE("Task put to sleep till " << duration_cast<milliseconds>(time_point.time_since_epoch()).count() << " ms");
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_timers.empty();
    }

    /// returns:
    /// - queue is empty  -> max()
    /// - no ready timers -> time_point of the next timer
    /// - some ready timers -> min()
    time_point_t poll(ready_sink_t& ready)
    {
        using namespace std::chrono;

        time_point_t now = clock_t::now();
        time_point_t next = clock_t::time_point::max();
        while (!m_timers.empty() && m_timers.top().time <= now)
        {
            auto [ tp, coro ] = m_timers.top();
            m_timers.pop();
            TRACE("Waking up timer task scheduled for " << duration_cast<milliseconds>(tp.time_since_epoch()).count()
                                                        << " ms");
            ready.push(coro);
            next = clock_t::time_point::min();
        }

        if (next != clock_t::time_point::min() && !m_timers.empty())
        {
            next = m_timers.top().time;
        }

        return next;
    }

private:
    std::priority_queue<timer> m_timers;
};

} // namespace co1::detail
