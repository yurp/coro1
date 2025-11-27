// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#include <co1/awaiters.hpp>
#include <co1/io_queue/select.hpp>
#include <co1/scheduler.hpp>

#include <iostream>

using select_scheduler = co1::scheduler<co1::io_queue::select>;

co1::task<void> logged_sleep(std::string tag, std::chrono::seconds duration)
{
    using namespace std::chrono;
    auto now = duration_cast<seconds>(steady_clock::now().time_since_epoch());
    std::cout << tag << now.count() << std::endl;
    co_await co1::wait(duration);
}

co1::task<void> async_main()
{
    using namespace std::chrono_literals;

    constexpr size_t BLINK_COUNT = 5;
    std::cout << "Starting blinking for " << BLINK_COUNT << " times" << std::endl;

    for (size_t i = 0; i < BLINK_COUNT; ++i)
    {
        co_await logged_sleep("LED ON:  ", 1s);
        co_await logged_sleep("LED OFF: ", 1s);
    }

    std::cout << "Blinking completed" << std::endl;
}

int main()
{
    try
    {
        select_scheduler{}.start(async_main());
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return 0;
}
