// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>

#include <chrono>
#include <coroutine>
#include <ranges>
#include <vector>
#include <utility>
#include <queue>

namespace co1
{

class timer_queue
{
public:
    using coro_t = std::coroutine_handle<>;

    struct timer
    {
        time_point_t time;
        coro_t coro;

        bool operator<(const timer& other) const { return time > other.time; }
    };

    void add(time_point_t time_point, coro_t coro)
    {
        using namespace std::chrono;
        m_timers.emplace(timer { time_point, coro });
        TRACE("Task put to sleep till " << duration_cast<milliseconds>(time_point.time_since_epoch()).count() << " ms");
    }

    /// returns time point of the next timer or time_point_t::max() if no timers are scheduled
    time_point_t poll(ready_sink& ready)
    {
        using namespace std::chrono;

        time_point_t now = clock_t::now();
        while (!m_timers.empty() && m_timers.top().time <= now)
        {
            auto [ tp, coro ] = m_timers.top();
            m_timers.pop();
            TRACE("Waking up timer task scheduled for " << duration_cast<milliseconds>(tp.time_since_epoch()).count()
                                                        << " ms");
            ready.push(coro);
        }
        return m_timers.empty() ? clock_t::now() : m_timers.top().time;
    }

private:
    std::priority_queue<timer> m_timers;
};

} // namespace co1
