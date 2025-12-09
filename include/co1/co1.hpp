// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/awaiters.hpp>
#include <co1/detail/timer_queue.hpp>
#include <co1/io_queue/select.hpp>
#include <co1/scheduler.hpp>
#include <co1/task.hpp>

namespace co1
{

template <unique_event_queues... Qs>
class basic_async_context
{
public:
    template <typename T>
    using task_t = basic_task<T, Qs...>;

    template <typename T>
    using task_handle_t = basic_task_handle<T, Qs...>;

    template <typename T>
    task_handle_t<T> spawn(task_t<T>&& task_to_spawn)
    {
        auto moved_task = std::move(task_to_spawn);
        auto coro_handle = moved_task.release();
        auto ctl = std::make_shared<detail::control_block>(coro_handle);
        coro_handle.promise().m_ctl = ctl;

        coro_handle.promise().m_context = &m_scheduler.get_pushable_context();
        m_scheduler.spawn(ctl);

        return task_handle_t<T> { std::move(ctl) };
    }

    template <typename T>
    T start(task_t<T>&& task_to_start)
    {
        auto tsk = spawn(std::move(task_to_start));
        run();
        return tsk.get();
    }

    void run()
    {
        TRACE("Starting scheduler loop");
        bool has_work = true;
        while (has_work)
        {
            has_work = m_scheduler.step();
        }
        TRACE("Ending scheduler loop");
    }

private:
    basic_scheduler<Qs...> m_scheduler;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using default_io_queue = io_queue::select;
using default_timer_queue = detail::timer_queue;

using async_context = basic_async_context<default_io_queue, default_timer_queue>;

template <typename T = void>
using task = async_context::task_t<T>;

template <typename T = void>
using task_handle = async_context::task_handle_t<T>;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class local_async_context
{
public:
    template <typename T>
    static T start(task<T>&& task_to_start)
    {
        return get_instance().start(std::move(task_to_start));
    }

    template <typename T>
    static task_handle<T> spawn(task<T>&& task_to_spawn)
    {
        return get_instance().spawn(std::move(task_to_spawn));
    }

    static void run()
    {
        get_instance().run();
    }

private:
    static async_context& get_instance()
    {
        static thread_local async_context instance;
        return instance;
    }
};

} // namespace co1
