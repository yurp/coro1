// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#include <co1/scheduler.hpp>
#include <co1/awaiters.hpp>
#include <co1/backend/select.hpp>

#include <iostream>

using select_scheduler = co1::scheduler<co1::backend::select>;

co1::task<void> logged_sleep(const std::string& tag, std::chrono::seconds duration)
{
    using namespace std::chrono;
    auto now = duration_cast<seconds>(steady_clock::now().time_since_epoch());
    std::cout << tag << now.count() << std::endl;
    co_await co1::sleep(duration);
}

co1::task<void> async_main()
{
    using namespace std::chrono_literals;
    for (size_t i = 0; i < 5; ++i)
    {
        co_await logged_sleep("LED ON:  ", 1s);
        co_await logged_sleep("LED OFF: ", 1s);
    }

    std::cout << "Blinking completed" << std::endl;
}

int main()
{
    select_scheduler{}.start(async_main());
    return 0;
}
