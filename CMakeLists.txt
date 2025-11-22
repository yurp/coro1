cmake_minimum_required(VERSION 3.28)
project(coro1 VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(co1 INTERFACE)
target_include_directories(co1 INTERFACE include)

add_subdirectory(examples)

option(CO1_BUILD_TESTS "Build unit tests" ON)

if (CO1_BUILD_TESTS)
    include(FetchContent)

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.4.0
    )

    FetchContent_MakeAvailable(Catch2)

    enable_testing()

    file(GLOB TEST_SOURCES tests/test_*.cpp)

    add_executable(co1_tests ${TEST_SOURCES})
    target_link_libraries(co1_tests PRIVATE co1 Catch2::Catch2WithMain)

    include(Catch)
    catch_discover_tests(co1_tests)
endif()
