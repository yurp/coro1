cmake_minimum_required(VERSION 3.28)
project(coro1 VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CO1_BUILD_TESTS "Build unit tests" OFF)
option(CO1_ENABLE_TRACE "Enable tracing functionality" OFF)

add_library(co1 INTERFACE)
target_include_directories(co1 INTERFACE include)
if(CO1_ENABLE_TRACE)
    target_compile_definitions(co1 INTERFACE CO1_ENABLE_TRACE)
endif()

add_subdirectory(examples)

if (CO1_BUILD_TESTS)
    find_package(Catch2 3.4 REQUIRED)

    enable_testing()

    set(TEST_SOURCES
        tests/test_exception.cpp
        tests/test_io_queue_select.cpp
        tests/test_queues.cpp
        tests/test_timer_queue.cpp
    )

    add_executable(co1_tests ${TEST_SOURCES})
    target_link_libraries(co1_tests PRIVATE co1 Catch2::Catch2WithMain)

    include(Catch)
    catch_discover_tests(co1_tests)
endif()
