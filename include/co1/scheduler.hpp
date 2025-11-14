// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>
#include <co1/context.hpp>
#include <co1/task.hpp>
#include <co1/timer_queue.hpp>

#include <queue>

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
        t.init(promise_context { &m_context });
        run();
        return t.get();
    }

    void run()
    {
        TRACE("Starting scheduler loop");
        while (true)
        {
            TRACE("Scheduler loop iteration");
            auto suspended_coro = m_context.coro;
            m_context.coro = nullptr;
            if (!suspended_coro)
            {
                TRACE("No more events to process, exiting scheduler loop");
                throw std::runtime_error("No more events to process");
            }

            auto payload = std::move(m_context.payload);
            m_context.payload = {};

            switch (payload.index())
            {
                case context::none:
                    TRACE("Payload isn't set");
                    throw std::runtime_error("Payload isn't set");

                case context::init:
                    TRACE("Processing init");
                    m_ready_coros.push(suspended_coro);
                    break;

                case context::finalize:
                    TRACE("Processing finalize");
                    return;

                case context::timer:
                {
                    TRACE("Processing timer");
                    time_point_t tp = std::get<context::timer>(payload).tp;
                    m_timer_queue.add(tp, suspended_coro);
                    break;
                }
                case context::io:
                {
                    TRACE("Processing IO event");
                    io_op op = std::get<context::io>(payload).operation;
                    m_poller.add(io_op { op.type, op.fd }, suspended_coro);
                    break;
                }
                default:
                    TRACE("Unknown event type");
                    throw std::runtime_error("Unknown event type");
            }

            ready_sink ready { &m_ready_coros };
            while (m_ready_coros.empty())
            {
                TRACE("No ready coroutines, polling timers and IO events");
                auto tp = m_timer_queue.poll(ready);
                auto now = clock_t::now();
                auto duration = (tp > now) ? tp - now : clock_t::duration::zero();
                if (m_ready_coros.empty() && duration == clock_t::duration::zero())
                    duration = std::chrono::seconds(24 * 3600); // lets wait day by day
                m_poller.poll(ready, duration);
            }

            TRACE("Resuming coroutine");
            auto coro_to_resume = m_ready_coros.front();
            m_ready_coros.pop();
            coro_to_resume.resume();
        }
        TRACE("Ending scheduler loop");
    }

private:
    context m_context;
    Poller m_poller;
    timer_queue m_timer_queue;

    std::queue<std::coroutine_handle<>> m_ready_coros;
};

} // namespace co1
