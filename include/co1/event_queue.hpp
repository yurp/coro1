// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/utils.hpp>

#include <cassert>
#include <chrono>
#include <coroutine>
#include <ranges>
#include <vector>
#include <utility>

namespace co1
{

class event_queue
{
public:
    using clock_t = std::chrono::steady_clock;
    using coro_t = std::coroutine_handle<>;

    struct timer
    {
        clock_t::time_point time;
        coro_t coro;
    };

    std::chrono::microseconds time_until_next_timer(clock_t::time_point now = clock_t::now()) const
    {
        if (m_timers.empty())
            return std::chrono::microseconds::max();

        auto next_timer = std::min_element(m_timers.begin(), m_timers.end(), [](const auto& a, const auto& b)
        {
            return a.time < b.time;
        });

        if (next_timer->time <= now)
            return std::chrono::microseconds(0);

        return std::chrono::duration_cast<std::chrono::microseconds>(next_timer->time - now);
    }

    template <typename Rep, typename Period>
    void add_timer(std::chrono::duration<Rep, Period> duration, coro_t coro)
    {
        using namespace std::chrono;
        m_timers.emplace_back(clock_t::now() + duration, coro);
        TRACE("Task put to sleep for " << duration_cast<milliseconds>(duration).count() << " ms");
    }

    void update_ready_timers(clock_t::time_point now = clock_t::now())
    {
        assert(m_ready_timers.empty() && "Ready timers should be processed before updating");
        m_ready_timers.reserve(m_timers.size());
        // preserve order of timers
        for (const auto& timer : m_timers | std::views::reverse)
            if (timer.time <= now)
                m_ready_timers.push_back(timer.coro);

        auto it = std::remove_if(m_timers.begin(), m_timers.end(), [this, now](const timer& event)
        {
            return event.time <= now;
        });
        m_timers.erase(it, m_timers.end());
    }

    coro_t pop_ready_timer()
    {
        if (m_ready_timers.empty())
            return nullptr;

        auto handle = m_ready_timers.back();
        m_ready_timers.pop_back();
        return handle;
    }

    bool empty() const
    {
        return m_timers.empty() && m_ready_timers.empty();
    }

private:
    std::vector<timer> m_timers;
    std::vector<coro_t> m_ready_timers;
};

} // namespace co1
