// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#include <co1/co1.hpp>

#include <catch2/catch_all.hpp>

using namespace std::chrono_literals;

namespace
{

co1::task<int> faulty_task()
{
    co_await co1::wait(100ms);
    throw std::runtime_error("Simulated task error");
    co_return 0;
}

} // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("exception test", "[exception]")
{
    co1::scheduler scheduler;

    SECTION("task handle catches exception")
    {
        REQUIRE_THROWS_WITH(scheduler.start(faulty_task()), "Simulated task error");
    }

    SECTION("catch child task exception in parent task")
    {
        int result = scheduler.start([]() -> co1::task<int>
        {
            try
            {
                co_return co_await faulty_task();
            }
            catch (const std::runtime_error& e)
            {
                TRACE(std::string("Caught exception in parent task: ") + e.what());
            }
            co_return -1;
        }());
        REQUIRE(result == -1);
    }

    SECTION("catch child task exception in grand parent task")
    {
        int result = scheduler.start([]() -> co1::task<int>
        {
            auto inner_task = []() -> co1::task<int> { co_return co_await faulty_task(); };
            try
            {
                co_return co_await inner_task();
            }
            catch (const std::runtime_error& e)
            {
                TRACE(std::string("Caught exception in parent task: ") + e.what());
            }
            co_return -1;
        }());
        REQUIRE(result == -1);
    }

    SECTION("catch handled exception from spawned task")
    {
        int result = scheduler.start([](auto* scheduler) -> co1::task<int>
        {
            auto hande = scheduler->spawn(faulty_task());
            co_await co1::wait(200ms); // wait for the task to fail
            try
            {
                co_return hande.get();
            }
            catch (const std::runtime_error& e)
            {
                TRACE(std::string("Caught exception in parent task: ") + e.what());
            }
            co_return -1;

        }(&scheduler));
        REQUIRE(result == -1);
    }

    SECTION("ignore handle of spawned task (no exception propagation like in std::future)")
    {
        int result = scheduler.start([](auto* scheduler) -> co1::task<int>
        {
            try
            {
                [[maybe_unused]] auto hande = scheduler->spawn(faulty_task());
                co_await co1::wait(200ms); // wait for the task to fail
                co_return 0;
            }
            catch (...)
            {
                REQUIRE(false); // should not reach here
            }

            co_return -1;
        }(&scheduler));
        REQUIRE(result == 0);
    }
}
