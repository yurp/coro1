// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#include <co1/detail/timer_queue.hpp>

#include <catch2/catch_all.hpp>

#include <thread>

using namespace std::chrono_literals;

TEST_CASE("timer_queue test", "[timer_queue]")
{
    co1::detail::timer_queue test_timer_queue {};
    co1::detail::ready_sink_t ready;
    REQUIRE(test_timer_queue.empty());

    SECTION("poll empty queue")
    {
        auto next_on_empty = test_timer_queue.poll(ready);
        REQUIRE(test_timer_queue.empty());
        REQUIRE(next_on_empty == co1::time_point_t::max());
    }

    SECTION("add and poll timer")
    {
        test_timer_queue.add(co1::clock_t::now() + 300ms, nullptr);
        REQUIRE(!test_timer_queue.empty());

        auto next_on_pending = test_timer_queue.poll(ready);
        REQUIRE(!test_timer_queue.empty());
        REQUIRE(next_on_pending <= co1::clock_t::now() + 300ms);

        std::this_thread::sleep_for(350ms);
        REQUIRE(!test_timer_queue.empty());
        auto next_on_ready = test_timer_queue.poll(ready);
        REQUIRE(test_timer_queue.empty());
        REQUIRE(next_on_ready == co1::time_point_t::min());
        REQUIRE(!ready.empty());
        REQUIRE(ready.front() == nullptr);

        auto next_on_empty = test_timer_queue.poll(ready);
        REQUIRE(test_timer_queue.empty());
        REQUIRE(next_on_empty == co1::time_point_t::max());
    }
}
