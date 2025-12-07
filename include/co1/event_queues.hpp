// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/common.hpp>

#include <concepts>

namespace co1
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail
{

template <typename Q>
concept has_input_type = requires { typename Q::input_type; };

template <typename Q>
concept has_output_type = requires { typename Q::output_type; };

template <typename Q>
concept has_result_type = requires { typename Q::result_type; };

template <typename Q>
concept has_empty_method = requires(const Q& queue) { { queue.empty() } -> std::same_as<bool>; };

template <typename Q>
concept pushable_with_input_and_result =
    has_input_type<Q> && has_result_type<Q> &&
    requires(Q& queue, typename Q::input_type input, typename Q::result_type& result, detail::coro_ctl ctl)
    {
        { queue.add(input, result, ctl) } -> std::same_as<void>;
    };

template <typename Q>
concept pushable_with_input_only =
    has_input_type<Q> &&
    requires(Q& queue, typename Q::input_type input, detail::coro_ctl ctl)
    {
        { queue.add(input, ctl) } -> std::same_as<void>;
    };

template <typename Q>
concept pollable_nonblocking =
    has_empty_method<Q> &&
    requires(Q& queue, detail::ready_sink_t& ready)
    {
        { queue.poll(ready) } -> std::same_as<time_point_t>;
    };

template <typename Q>
concept pollable_blocking =
    has_empty_method<Q> &&
    requires(Q& queue, detail::ready_sink_t& ready)
    {
        { queue.poll(ready, std::chrono::milliseconds{}) } -> std::same_as<std::error_code>;
    };

template<typename Q>
concept pollable_event_queue = pollable_nonblocking<Q> || pollable_blocking<Q>;

} // namespace detail

template <typename Q>
concept pushable_event_queue = detail::pushable_with_input_and_result<Q> || detail::pushable_with_input_only<Q>;

template<typename Q>
concept blocking_event_queue = std::default_initializable<Q> &&
                               pushable_event_queue<Q> &&
                               detail::pollable_blocking<Q>;

template<typename Q>
concept generic_event_queue = std::default_initializable<Q> &&
                              pushable_event_queue<Q> &&
                              detail::pollable_nonblocking<Q>;

template<typename Q>
concept event_queue = blocking_event_queue<Q> || generic_event_queue<Q>;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename... Ts>
constexpr bool appears_once_v = (0 + ... + (std::is_same_v<T, Ts> ? 1 : 0)) == 1;

template<event_queue... Qs>
constexpr bool unique_event_queues_by_input_type_v = (appears_once_v<typename Qs::input_type,
                                                                     typename Qs::input_type...> && ...);

template<event_queue... Qs>
constexpr bool has_at_most_one_blocking_event_queue_v = (0 + ... + (blocking_event_queue<Qs> ? 1 : 0)) <= 1;

template<event_queue... Qs>
constexpr bool has_blocking_event_queue_v = (0 + ... + (blocking_event_queue<Qs> ? 1 : 0)) > 0;

template <typename... Qs>
concept unique_event_queues = (event_queue<Qs> && ...) &&
                              unique_event_queues_by_input_type_v<Qs...> &&
                              has_at_most_one_blocking_event_queue_v<Qs...>;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename In, unique_event_queues... Qs>
consteval size_t find_queue_index()
{
    constexpr std::array<bool, sizeof...(Qs)> matches = { std::is_same_v<typename Qs::input_type, In>... };
    for (size_t i = 0; i < sizeof...(Qs); ++i)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        if (matches[i])
        {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

template <typename In, unique_event_queues... Qs>
auto& get_event_queue(std::tuple<Qs...>& queues)
{
    constexpr size_t idx = find_queue_index<In, Qs...>();
    static_assert(idx != static_cast<size_t>(-1), "No queue with given input_type");
    return std::get<idx>(queues);
}

template <typename In, unique_event_queues... Qs>
void add_to_queue(std::tuple<Qs...>& queues, In&& input, detail::coro_ctl ctl)
{
    using input_t = std::remove_cvref_t<In>;
    get_event_queue<input_t, Qs...>(queues).add(std::forward<In>(input), ctl);
}

template <typename In, typename Out, unique_event_queues... Qs>
void add_to_queue(std::tuple<Qs...>& queues, In&& input, Out&& output, detail::coro_ctl ctl)
{
    using input_t = std::remove_cvref_t<In>;
    get_event_queue<input_t, Qs...>(queues).add(std::forward<In>(input), std::forward<Out>(output), ctl);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <unique_event_queues... Qs>
consteval size_t find_io_queue_index()
{
    constexpr std::array<bool, sizeof...(Qs)> matches = { blocking_event_queue<Qs>... };
    for (size_t i = 0; i < sizeof...(Qs); ++i)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        if (matches[i])
        {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

template<unique_event_queues... Qs>
auto& get_io_event_queue(std::tuple<Qs...>& queues)
{
    constexpr size_t idx = find_io_queue_index<Qs...>();
    static_assert(idx != static_cast<size_t>(-1), "No io_event_queue present in the queue set");
    return std::get<idx>(queues);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <unique_event_queues... Qs>
time_point_t poll_generic_event_queues(detail::ready_sink_t& sink, std::tuple<Qs...>& queues)
{
    time_point_t result = time_point_t::max();
    ([&]
    {
        if constexpr (generic_event_queue<Qs>)
        {
            result = std::min(result, std::get<Qs>(queues).poll(sink));
        }
    }(), ...);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <unique_event_queues... Qs>
bool event_queues_empty(std::tuple<Qs...>& queues) { return (std::get<Qs>(queues).empty() && ...); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <unique_event_queues... Qs>
struct pushable_context
{
    std::tuple<Qs...> m_queues {};
    detail::coro_ctl m_finalized_coro = nullptr;
};

template <typename Ctx, typename In>
concept pushable = requires(Ctx ctx, In&& input)
{
    { ctx.push_to_queue(std::forward<In>(input)) } -> std::same_as<void>;
};

template <typename Ctx, typename In, typename Result>
concept pushable_with_result = requires(Ctx ctx, In&& input, Result&& output)
{
    { ctx.push_to_queue(std::forward<In>(input), std::forward<Result>(output)) } -> std::same_as<void>;
};

} // namespace co1
