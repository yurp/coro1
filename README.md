# coro1

[![CI](https://github.com/yurp/coro1/actions/workflows/ci.yml/badge.svg)](https://github.com/yurp/coro1/actions/workflows/ci.yml)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Single-threaded header-only C++20 coroutine framework for embedded and edge microservices

### [Usage example](examples/blink.cpp)

```c++
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

```
