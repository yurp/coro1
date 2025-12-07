// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#include <co1/event_queues.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("appears_once_v test", "[queues]")
{
    REQUIRE(co1::appears_once_v<int> == false);
    REQUIRE(co1::appears_once_v<int, int> == true);
    REQUIRE(co1::appears_once_v<int, int, int> == false);
    REQUIRE(co1::appears_once_v<int, int, bool> == true);
    REQUIRE(co1::appears_once_v<int, bool, int> == true);
    REQUIRE(co1::appears_once_v<int, bool, int, int> == false);
    REQUIRE(co1::appears_once_v<int, bool, double> == false);
    REQUIRE(co1::appears_once_v<char, int, int, int, bool, double> == false);
    REQUIRE(co1::appears_once_v<char, int, int, int, bool, double, char> == true);

    using int_t = int;
    REQUIRE(co1::appears_once_v<int, int, int_t> == false);
}

struct base_queue
{
    int magic_number = 0;
};

struct base_io_queue : base_queue
{
    std::error_code poll(co1::detail::ready_sink_t& /*unused*/, std::chrono::milliseconds /*unused*/) { return {}; }
};

struct queue0 : base_queue
{
    using input_type = int;
    using result_type = void;

    void add(input_type /*unused*/, const co1::detail::coro_ctl& /*unused*/) { }
    [[nodiscard]] bool empty() const noexcept { return true; }

    co1::time_point_t poll(co1::detail::ready_sink_t& /*unused*/)
    {
        return co1::time_point_t::max();
    }
};

struct queue1 : base_queue
{
    using input_type = int;
    using result_type = void;

    void add(input_type /*unused*/, const co1::detail::coro_ctl& /*unused*/) { }
    [[nodiscard]] bool empty() const noexcept { return false; }

    co1::time_point_t poll(co1::detail::ready_sink_t& ready)
    {
        constexpr int MAGIC_DURATION_MS = 42;
        ready.push(nullptr);
        return co1::time_point_t { std::chrono::milliseconds{ MAGIC_DURATION_MS } };
    }
};

struct queue1x : base_queue
{
    using input_type = int;
    using result_type = double;

    void add(input_type /*unused*/, result_type& /*unused*/, const co1::detail::coro_ctl& /*unused*/) { }
    [[nodiscard]] bool empty() const noexcept { return false; }

    co1::time_point_t poll(co1::detail::ready_sink_t& ready)
    {
        constexpr int MAGIC_DURATION_MS = 0;
        ready.push(nullptr);
        ready.push(nullptr);
        return co1::time_point_t { std::chrono::milliseconds{ MAGIC_DURATION_MS } };
    }
};

struct queue2 : base_queue
{
    using input_type = double;
    using result_type = double;

    void add(input_type /*unused*/, result_type& /*unused*/, const co1::detail::coro_ctl& /*unused*/) { }
    [[nodiscard]] bool empty() const noexcept { return false; }

    co1::time_point_t poll(co1::detail::ready_sink_t& ready)
    {
        constexpr int MAGIC_DURATION_MS = 7;
        ready.push(nullptr);
        ready.push(nullptr);
        ready.push(nullptr);
        ready.push(nullptr);
        ready.push(nullptr);
        ready.push(nullptr);
        ready.push(nullptr);
        return co1::time_point_t { std::chrono::milliseconds{ MAGIC_DURATION_MS } };
    }
};

struct queue_io : base_io_queue
{
    using input_type = char;
    using result_type = std::error_code;

    void add(input_type /*unused*/, result_type& /*unused*/, const co1::detail::coro_ctl& /*unused*/) { }
    [[nodiscard]] bool empty() const noexcept { return true; }
};

struct another_queue_io : base_io_queue
{
    using input_type = co1::io_wait;
    using result_type = std::error_code;

    void add(input_type /*unused*/, result_type& /*unused*/, const co1::detail::coro_ctl& /*unused*/) { }
    [[nodiscard]] bool empty() const noexcept { return true; }
};

TEST_CASE("event_queues_empty() test", "[queues]")
{
    std::tuple<> queues0;
    REQUIRE(co1::event_queues_empty(queues0) == true);

    std::tuple<queue0, queue_io> queues1;
    REQUIRE(co1::event_queues_empty(queues1) == true);

    std::tuple<queue0> queues2;
    REQUIRE(co1::event_queues_empty(queues2) == true);

    std::tuple<queue_io> queues3;
    REQUIRE(co1::event_queues_empty(queues3) == true);

    std::tuple<queue1> queues4;
    REQUIRE(co1::event_queues_empty(queues4) == false);

    std::tuple<queue1, queue2, queue_io> queues5;
    REQUIRE(co1::event_queues_empty(queues5) == false);

    std::tuple<queue1x, queue2, another_queue_io> queues6;
    REQUIRE(co1::event_queues_empty(queues6) == false);

    std::tuple<queue1x, queue2> queues7;
    REQUIRE(co1::event_queues_empty(queues7) == false);
}

TEST_CASE("poll_generic_event_queues() test", "[queues]")
{
    std::tuple<> queues0;
    co1::detail::ready_sink_t ready_sink0;
    auto next_time_point0 = co1::poll_generic_event_queues(ready_sink0, queues0);
    REQUIRE(next_time_point0 == co1::time_point_t::max());
    REQUIRE(ready_sink0.empty() == true); // no queues

    std::tuple<queue1, queue2, queue_io> queues1;
    co1::detail::ready_sink_t ready_sink1;
    auto next_time_point1 = co1::poll_generic_event_queues(ready_sink1, queues1);
    REQUIRE(next_time_point1 == co1::time_point_t { std::chrono::milliseconds{ 7 } });
    REQUIRE(ready_sink1.size() == 8); // 1 from queue1 + 7 from queue2 + 0 from queue_io

    std::tuple<queue1x, queue2, queue_io> queues2;
    co1::detail::ready_sink_t ready_sink2;
    auto next_time_point2 = co1::poll_generic_event_queues(ready_sink2, queues2);
    REQUIRE(next_time_point2 == co1::time_point_t { std::chrono::milliseconds{ 0 } });
    REQUIRE(ready_sink2.size() == 9); // 2 from queue1x + 7 from queue2 + 0 from queue_io

    std::tuple<queue_io, queue1x, queue2> queues3;
    co1::detail::ready_sink_t ready_sink3;
    auto next_time_point3 = co1::poll_generic_event_queues(ready_sink3, queues3);
    REQUIRE(next_time_point3 == co1::time_point_t { std::chrono::milliseconds{ 0 } });
    REQUIRE(ready_sink3.size() == 9); // 0 from queue_io + 2 from queue1x + 7 from queue2

    std::tuple<queue1x, queue_io, queue2> queues4;
    co1::detail::ready_sink_t ready_sink4;
    auto next_time_point4 = co1::poll_generic_event_queues(ready_sink4, queues4);
    REQUIRE(next_time_point4 == co1::time_point_t { std::chrono::milliseconds{ 0 } });
    REQUIRE(ready_sink4.size() == 9); // 0 from queue_io + 2 from queue1x + 7 from queue2

    std::tuple<queue1x, queue2> queues5;
    co1::detail::ready_sink_t ready_sink5;
    auto next_time_point5 = co1::poll_generic_event_queues(ready_sink5, queues5);
    REQUIRE(next_time_point5 == co1::time_point_t { std::chrono::milliseconds{ 0 } });
    REQUIRE(ready_sink5.size() == 9); // 2 from queue1x + 7 from queue2

    std::tuple<queue1, queue_io> queues6;
    co1::detail::ready_sink_t ready_sink6;
    auto next_time_point6 = co1::poll_generic_event_queues(ready_sink6, queues6);
    REQUIRE(next_time_point6 == co1::time_point_t { std::chrono::milliseconds{ 42 } });
    REQUIRE(ready_sink6.size() == 1); // 1 from queue1 + 0 from queue_io

    std::tuple<queue1> queues7;
    co1::detail::ready_sink_t ready_sink7;
    auto next_time_point7 = co1::poll_generic_event_queues(ready_sink7, queues7);
    REQUIRE(next_time_point7 == co1::time_point_t { std::chrono::milliseconds{ 42 } });
    REQUIRE(ready_sink7.size() == 1); // 1 from queue1

    std::tuple<queue_io> queues8;
    co1::detail::ready_sink_t ready_sink8;
    auto next_time_point8 = co1::poll_generic_event_queues(ready_sink8, queues8);
    REQUIRE(next_time_point8 == co1::time_point_t::max());
    REQUIRE(ready_sink8.empty() == true); // 0 from queue_io
}

TEST_CASE("get_queue() test", "[queues]")
{
    constexpr int MAGIC_NUMBER_0 = 42;
    std::tuple<queue1, queue2, queue_io> queues;

    auto& queue0 = co1::get_event_queue<int>(queues);
    queue0.magic_number = MAGIC_NUMBER_0;
    REQUIRE(std::get<0>(queues).magic_number == MAGIC_NUMBER_0);

    constexpr int MAGIC_NUMBER_1 = 84;
    auto& queue1 = co1::get_event_queue<double>(queues);
    queue1.magic_number = MAGIC_NUMBER_1;
    REQUIRE(std::get<1>(queues).magic_number == MAGIC_NUMBER_1);

    constexpr int MAGIC_NUMBER_2 = 127;
    auto& queue2 = co1::get_event_queue<char>(queues);
    queue2.magic_number = MAGIC_NUMBER_2;
    REQUIRE(std::get<2>(queues).magic_number == MAGIC_NUMBER_2);
}

TEST_CASE("get_io_event_queue() test", "[queues]")
{
    constexpr int MAGIC_NUMBER = 42;
    std::tuple<queue1, queue2, queue_io> queues;

    REQUIRE(co1::has_blocking_event_queue_v<queue1, queue2, queue_io> == true);
    auto& queue = co1::get_io_event_queue(queues);
    queue.magic_number = MAGIC_NUMBER;
    REQUIRE(std::get<0>(queues).magic_number == 0);
    REQUIRE(std::get<1>(queues).magic_number == 0);
    REQUIRE(std::get<2>(queues).magic_number == MAGIC_NUMBER);

    REQUIRE(co1::has_blocking_event_queue_v<queue_io> == true);
    REQUIRE(co1::has_blocking_event_queue_v<> == false);
    REQUIRE(co1::has_blocking_event_queue_v<queue1, queue2> == false);
}

TEST_CASE("unique_queues_by_input_type_v test", "[queues]")
{
    REQUIRE(co1::unique_event_queues_by_input_type_v<> == true);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue1> == true);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue1x> == true);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue2> == true);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue_io> == true);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue1, queue2> == true);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue1x, queue2> == true);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue1, queue_io> == true);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue2, queue_io> == true);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue1, queue2, queue_io> == true);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue_io, another_queue_io, queue1> == true);

    REQUIRE(co1::unique_event_queues_by_input_type_v<queue1, queue1x> == false);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue1, queue_io, queue1> == false);
    REQUIRE(co1::unique_event_queues_by_input_type_v<queue1, queue2, queue_io, queue2> == false);
}

TEST_CASE("has_zero_or_one_io_queue test", "[queues]")
{
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue1> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue1x> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue2> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue_io> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue1, queue2> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue1x, queue2> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue1, queue_io> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue2, queue_io> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue1, queue2, queue_io> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue_io, queue1, queue2> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue_io, queue1, queue1, queue2> == true);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue_io, queue1, queue1, queue2, queue2, queue1x> == true);

    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue1, queue2, queue_io, queue_io> == false);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue_io, another_queue_io, queue1> == false);
    REQUIRE(co1::has_at_most_one_blocking_event_queue_v<queue_io, queue1, another_queue_io> == false);
}

TEST_CASE("QueueTuple test", "[queues]")
{
    REQUIRE(co1::unique_event_queues<>);
    REQUIRE(co1::unique_event_queues<queue1>);
    REQUIRE(co1::unique_event_queues<queue1x>);
    REQUIRE(co1::unique_event_queues<queue2>);
    REQUIRE(co1::unique_event_queues<queue_io>);
    REQUIRE(co1::unique_event_queues<queue1, queue2>);
    REQUIRE(co1::unique_event_queues<queue1x, queue2>);
    REQUIRE(co1::unique_event_queues<queue1, queue_io>);
    REQUIRE(co1::unique_event_queues<queue2, queue_io>);
    REQUIRE(co1::unique_event_queues<queue1, queue2, queue_io>);

    REQUIRE(!co1::unique_event_queues<queue1, queue1x>);
    REQUIRE(!co1::unique_event_queues<queue1, queue_io, queue1>);
    REQUIRE(!co1::unique_event_queues<queue1, queue2, queue_io, queue2>);
    REQUIRE(!co1::unique_event_queues<queue1, queue2, queue_io, queue_io>);
    REQUIRE(!co1::unique_event_queues<queue_io, another_queue_io, queue1>);
    REQUIRE(!co1::unique_event_queues<queue_io, queue1, another_queue_io>);
}

TEST_CASE("concepts test", "[concepts]")
{
    REQUIRE(co1::detail::pushable_with_input_only<queue1>);
    REQUIRE(co1::detail::pushable_with_input_and_result<queue1x>);
    REQUIRE(co1::detail::pushable_with_input_and_result<queue2>);
    REQUIRE(co1::detail::pushable_with_input_and_result<queue_io>);

    REQUIRE(co1::detail::pollable_nonblocking<queue1>);
    REQUIRE(co1::detail::pollable_nonblocking<queue1x>);
    REQUIRE(co1::detail::pollable_nonblocking<queue2>);
    REQUIRE(co1::detail::pollable_blocking<queue_io>);

    REQUIRE(co1::generic_event_queue<queue1>);
    REQUIRE(co1::generic_event_queue<queue1x>);
    REQUIRE(co1::generic_event_queue<queue2>);
    REQUIRE(co1::blocking_event_queue<queue_io>);
}

TEST_CASE("queue concept negative test", "[concepts]")
{
    struct bad_queue1
    {
        using input_type = int;
        using result_type = void;

        // missing add()

        [[nodiscard]] bool empty() const noexcept { return true; }
        co1::time_point_t poll(co1::detail::ready_sink_t& /*unused*/) { return { }; }
    };
    REQUIRE(!co1::pushable_event_queue<bad_queue1>);

    struct bad_queue2
    {
        using input_type = int;
        using result_type = void;

        void add(input_type /*unused*/, const co1::detail::coro_ctl& /*unused*/) { }
        // missing empty()

        co1::time_point_t poll(co1::detail::ready_sink_t& /*unused*/) { return { }; }
    };
    REQUIRE(!co1::event_queue<bad_queue2>);

    struct bad_queue3
    {
        using input_type = int;

        void add(input_type /*unused*/, const co1::detail::coro_ctl& /*unused*/) { }
        [[nodiscard]] bool empty() const noexcept { return true; }
        // missing poll()

    };
    REQUIRE(!co1::detail::pollable_event_queue<bad_queue3>);
}
