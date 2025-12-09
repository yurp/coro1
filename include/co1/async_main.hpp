// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#include <co1/co1.hpp>

#include <iostream>

co1::task<int> async_main(int argc, char** argv);

int main(int argc, char* argv[])
{
    try
    {
        co1::local_async_context async_ctx;
        int result = async_ctx.start(async_main(argc, argv));
        return result;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unhandled exception in async_main: " << e.what() << std::endl;
        return -1;
    }
}
