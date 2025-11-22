// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#include <co1/detail/timer_queue.hpp>

#include <catch2/catch_all.hpp>

#include <thread>

using namespace std::chrono_literals;

TEST_CASE("timer_queue test", "[timer_queue]")
{
    co1::detail::timer_queue tq {};
    co1::detail::ready_sink_t ready;

    REQUIRE(tq.empty());

    SECTION("poll empty queue")
    {
        auto next_on_empty = tq.poll(ready);
        REQUIRE(tq.empty());
        REQUIRE(next_on_empty == co1::time_point_t::max());
    }

    SECTION("add and poll timer")
    {
        tq.add(co1::clock_t::now() + 300ms, std::noop_coroutine());
        REQUIRE(!tq.empty());

        auto next_on_pending = tq.poll(ready);
        REQUIRE(!tq.empty());
        REQUIRE(next_on_pending <= co1::clock_t::now() + 300ms);

        std::this_thread::sleep_for(350ms);
        REQUIRE(!tq.empty());

        auto next_on_ready = tq.poll(ready);
        REQUIRE(tq.empty());
        REQUIRE(next_on_ready == co1::time_point_t::min());
        REQUIRE(!ready.empty());
        REQUIRE(ready.front() == std::noop_coroutine());

        auto next_on_empty = tq.poll(ready);
        REQUIRE(tq.empty());
        REQUIRE(next_on_empty == co1::time_point_t::max());
    }
}
