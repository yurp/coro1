// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#include <co1/io_queue/select.hpp>

#include <catch2/catch_all.hpp>

#include <sys/eventfd.h>

#include <thread>

using namespace std::chrono_literals;

namespace
{

constexpr uint64_t MAGIC_NUMBER = 42;
constexpr uint64_t MAGIC_NUMBER_1 = 21;
constexpr uint64_t MAGIC_NUMBER_2 = 42;

co1::fd_t create_eventfd()
{
    co1::fd_t event_fd = ::eventfd(0, EFD_NONBLOCK);
    REQUIRE(event_fd != -1);
    return event_fd;
}

int64_t read_eventfd(co1::fd_t event_fd)
{
    int64_t value = 0;
    ssize_t bytes_read = ::read(event_fd, &value, sizeof(int64_t));
    REQUIRE(bytes_read == sizeof(int64_t));
    return value;
}

void write_eventfd(co1::fd_t event_fd, int64_t value)
{
    ssize_t bytes_written = ::write(event_fd, &value, sizeof(int64_t));
    REQUIRE(bytes_written == sizeof(int64_t));
}

template<typename Rep, typename Period>
co1::time_point_t get_future_timepoint(std::chrono::duration<Rep, Period> duration)
{
    return co1::clock_t::now() + duration;
}

} // namespace

TEST_CASE("io_queue/select simple: empty and invalid fd scenarios", "[io_queue/select]")
{
    co1::io_queue::select test_select {};
    co1::detail::ready_sink_t ready;
    co1::poll_context poll_ctx { /* finalized coro*/ nullptr, co1::clock_t::now() };

    REQUIRE(test_select.empty());
    WHEN("polling empty queue")
    {
        std::error_code ecode = test_select.poll(ready, poll_ctx, get_future_timepoint(1ms));
        THEN("poll returns immediately with no error")
        {
            REQUIRE(test_select.empty());
            REQUIRE(!ecode);
            REQUIRE(ready.empty());
        }
    }
    WHEN("there is an invalid fd")
    {
        co1::fd_t invalid_fd = -1;
        std::error_code fd_error;
        WHEN("adding invalid fd to the queue")
        {
            test_select.add({ co1::io_type::read, invalid_fd }, fd_error, nullptr);
            THEN("element is added to the queue")
            {
                REQUIRE(!test_select.empty());
            }
            WHEN("polling the queue with invalid fd")
            {
                std::error_code ecode = test_select.poll(ready, poll_ctx, get_future_timepoint(1ms));
                THEN("empty queue, no error from poll (error is reported via fd_error), 'ready' contains the coro")
                {
                    REQUIRE(test_select.empty());
                    REQUIRE(!ecode);
                    REQUIRE(!ready.empty());
                    REQUIRE(ready.front() == nullptr);
                    REQUIRE(fd_error == std::errc::invalid_argument);
                }
            }
        }
    }
}

TEST_CASE("io_queue/select one fd scenarios", "[io_queue/select]")
{
    co1::io_queue::select test_select {};
    co1::detail::ready_sink_t ready;
    co1::poll_context poll_ctx { /* finalized coro*/ nullptr, co1::clock_t::now() };

    WHEN("there is a valid fd")
    {
        co1::fd_t event_fd = create_eventfd();
        std::error_code fd_error;
        WHEN("adding valid fd to the queue for read")
        {
            test_select.add({ co1::io_type::read, event_fd }, fd_error, nullptr);
            REQUIRE(!test_select.empty());
            WHEN("writing data to eventfd to make it readable and polling immediately (it is now readable)")
            {
                write_eventfd(event_fd, MAGIC_NUMBER);
                std::error_code ecode = test_select.poll(ready, poll_ctx, get_future_timepoint(100ms));
                THEN("empty queue, no errors, coro is in 'ready', read from fd succeeded")
                {
                    REQUIRE(test_select.empty());
                    REQUIRE(!ecode);
                    REQUIRE(!ready.empty());
                    REQUIRE(ready.front() == nullptr);
                    REQUIRE(!fd_error);
                    REQUIRE(read_eventfd(event_fd) == MAGIC_NUMBER);
                }
            }
            WHEN("polling with timeout but added fd is not ready for read")
            {
                std::error_code ecode = test_select.poll(ready, poll_ctx, get_future_timepoint(100ms));
                THEN("poll returns after timeout with no ready fds")
                {
                    REQUIRE(!ecode);
                    REQUIRE(ready.empty());
                    REQUIRE(!fd_error);
                }
                THEN("the fd is still in the queue")
                {
                    REQUIRE(!test_select.empty());
                }
            }
        }
        WHEN("adding valid fd to the queue for write and polling immediately (it is writable)")
        {
            test_select.add({ co1::io_type::write, event_fd }, fd_error, nullptr);
            REQUIRE(!test_select.empty());
            std::error_code ecode = test_select.poll(ready, poll_ctx, get_future_timepoint(100ms));
            THEN("empty queue, no errors, coro is in 'ready'")
            {
                REQUIRE(test_select.empty());
                REQUIRE(!ecode);
                REQUIRE(!ready.empty());
                REQUIRE(ready.front() == nullptr);
                REQUIRE(!fd_error);
            }
        }
        ::close(event_fd);
    }
}

TEST_CASE("io_queue/select two fds scenarios", "[io_queue/select]")
{
    co1::io_queue::select test_select {};
    co1::detail::ready_sink_t ready;
    co1::poll_context poll_ctx { /* finalized coro*/ nullptr, co1::clock_t::now() };

    WHEN("there are 2 valid fds")
    {
        co1::fd_t event_fd1 = create_eventfd();
        co1::fd_t event_fd2 = create_eventfd();
        std::error_code fd_error1;
        std::error_code fd_error2;
        WHEN("adding both fds to the queue for read")
        {
            test_select.add({ co1::io_type::read, event_fd1 }, fd_error1, nullptr);
            test_select.add({ co1::io_type::read, event_fd2 }, fd_error2, nullptr);
            REQUIRE(!test_select.empty());
            WHEN("writing data to 1st eventfd to make it readable")
            {
                write_eventfd(event_fd1, MAGIC_NUMBER_1);
                WHEN("polling the queue")
                {
                    std::error_code ecode = test_select.poll(ready, poll_ctx, get_future_timepoint(100ms));
                    THEN("no errors, 2nd fd still pending, 1st fd ready, read 1st fd succeeds")
                    {
                        REQUIRE(!ecode);
                        REQUIRE(!fd_error1);
                        REQUIRE(!fd_error2);
                        REQUIRE(!test_select.empty());
                        REQUIRE(!ready.empty());
                        REQUIRE(ready.front() == nullptr);
                        REQUIRE(read_eventfd(event_fd1) == MAGIC_NUMBER_1);
                    }
                    WHEN("writing data to 2nd eventfd to make it readable and polling again")
                    {
                        write_eventfd(event_fd2, MAGIC_NUMBER_2);
                        ecode = test_select.poll(ready, poll_ctx, get_future_timepoint(100ms));
                        THEN("no errors, empty queue, 2nd fd ready, read 2nd fd succeeds")
                        {
                            REQUIRE(!ecode);
                            REQUIRE(!fd_error2);
                            REQUIRE(test_select.empty());
                            REQUIRE(!ready.empty());
                            REQUIRE(ready.front() == nullptr);
                            REQUIRE(read_eventfd(event_fd2) == MAGIC_NUMBER_2);
                        }
                    }
                }
                WHEN("writing data to 2nd eventfd to make it readable immediately after 1st, then polling the queue")
                {
                    write_eventfd(event_fd2, MAGIC_NUMBER_2);
                    std::error_code ecode = test_select.poll(ready, poll_ctx, get_future_timepoint(100ms));
                    THEN("no errors, queue becomes empty, both fds ready, read from both fds succeeds")
                    {
                        REQUIRE(!ecode);
                        REQUIRE(!fd_error1);
                        REQUIRE(!fd_error2);
                        REQUIRE(test_select.empty());
                        REQUIRE(ready.size() == 2);
                        REQUIRE(ready.front() == nullptr);
                        ready.pop();
                        REQUIRE(ready.front() == nullptr);
                        REQUIRE(read_eventfd(event_fd1) == MAGIC_NUMBER_1);
                        REQUIRE(read_eventfd(event_fd2) == MAGIC_NUMBER_2);
                    }
                }
            }
        }
        ::close(event_fd1);
        ::close(event_fd2);
    }
}

TEST_CASE("io_queue/select complex scenario", "[io_queue/select]")
{
    co1::io_queue::select test_select {};
    co1::detail::ready_sink_t ready;
    co1::poll_context poll_ctx { /* finalized coro*/ nullptr, co1::clock_t::now() };

    WHEN("there are 2 fds, add them both to the queue for read")
    {
        co1::fd_t event_fd1 = create_eventfd();
        co1::fd_t event_fd2 = create_eventfd();
        std::error_code fd_error1;
        std::error_code fd_error2;

        test_select.add({ co1::io_type::read, event_fd1 }, fd_error1, nullptr);
        test_select.add({ co1::io_type::read, event_fd2 }, fd_error2, nullptr);
        WHEN("write to first eventfd to make it readable, and then poll the queue")
        {
            write_eventfd(event_fd1, MAGIC_NUMBER_1);
            std::error_code ecode = test_select.poll(ready, poll_ctx, get_future_timepoint(100ms));
            THEN("no errors, 2nd fd still pending, 1st fd ready, read 1st fd succeeds")
            {
                REQUIRE(!ecode);
                REQUIRE(!fd_error1);
                REQUIRE(!fd_error2);
                REQUIRE(!test_select.empty());
                REQUIRE(!ready.empty());
                REQUIRE(ready.front() == nullptr);
                REQUIRE(read_eventfd(event_fd1) == MAGIC_NUMBER_1);

                uint64_t val = 0;
                ssize_t bytes_read = ::read(event_fd2, &val, sizeof(uint64_t));
                REQUIRE(bytes_read == -1);
                REQUIRE(errno == EAGAIN);
            }
            ready.pop();
            REQUIRE(ready.empty());
            WHEN("close 1st, add the closed 1st back to the queue, write to 2nd to make it readable, poll the queue")
            {
                ::close(event_fd1);
                test_select.add({ co1::io_type::read, event_fd1 }, fd_error1, nullptr);
                write_eventfd(event_fd2, MAGIC_NUMBER_2);
                ecode = test_select.poll(ready, poll_ctx, get_future_timepoint(100ms));
                THEN("both fds are in 'ready', but closed fd reports error, read of 2nd fd succeeds")
                {
                    REQUIRE(!ecode);
                    REQUIRE(fd_error1 == std::errc::bad_file_descriptor);
                    REQUIRE(!fd_error2);
                    REQUIRE(test_select.empty());
                    REQUIRE(ready.size() == 2);
                    REQUIRE(ready.front() == nullptr);
                    ready.pop();
                    REQUIRE(ready.front() == nullptr);
                    REQUIRE(read_eventfd(event_fd2) == MAGIC_NUMBER_2);
                }
            }

        }
        ::close(event_fd2);
    }
}
