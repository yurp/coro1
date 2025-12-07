// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/awaiters.hpp>
#include <co1/detail/timer_queue.hpp>
#include <co1/io_queue/select.hpp>
#include <co1/scheduler.hpp>

namespace co1
{

using default_io_queue = io_queue::select;
using default_timer_queue = detail::timer_queue;

using scheduler = basic_scheduler<default_io_queue, default_timer_queue>;

template <typename T = void>
using task = scheduler::task_t<T>;

template <typename T = void>
using task_handle = scheduler::task_handle_t<T>;

} // namespace co1
