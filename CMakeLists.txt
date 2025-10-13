cmake_minimum_required(VERSION 3.28)
project(coro1 VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(co1 INTERFACE)
target_include_directories(co1 INTERFACE include)

add_subdirectory(examples)


