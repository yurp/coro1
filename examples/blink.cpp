// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#include <co1/async_main.hpp>

#include <iostream>

co1::task<> logged_sleep(std::string tag, std::chrono::seconds duration)
{
    using namespace std::chrono;
    auto now = duration_cast<seconds>(steady_clock::now().time_since_epoch());
    std::cout << tag << now.count() << std::endl;
    co_await co1::wait(duration);
}

co1::task<int> async_main(int /*argc*/, char** /*argv*/)
{
    using namespace std::chrono_literals;

    constexpr int MAGIC_NUMBER = 42;
    constexpr size_t BLINK_COUNT = 5;
    std::cout << "Starting blinking for " << BLINK_COUNT << " times" << std::endl;

    for (size_t i = 0; i < BLINK_COUNT; ++i)
    {
        co_await logged_sleep("LED ON:  ", 1s);
        co_await logged_sleep("LED OFF: ", 1s);
    }

    std::cout << "Blinking completed" << std::endl;
    co_return MAGIC_NUMBER;
}
