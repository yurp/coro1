// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#include <co1/scheduler.hpp>
#include <co1/backend/select.hpp>

#include <iostream>

using select_scheduler = co1::scheduler<co1::backend::select>;

std::chrono::seconds now()
{
    using namespace std::chrono;
    return duration_cast<seconds>(steady_clock::now().time_since_epoch());
}

co1::task<int> async_main()
{
    using namespace std::chrono_literals;
    for (size_t i = 0; i < 5; ++i)
    {
        std::cout << "LED ON: " << now().count() << std::endl;
        co_await 1s;
        std::cout << "LED OFF: " << now().count() << std::endl;
        co_await 1s;
    }

    std::cout << "Blinking completed" << std::endl;
    co_return 0;
}

int main()
{
    return select_scheduler{}.start(async_main);
}
