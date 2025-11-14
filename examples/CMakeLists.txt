add_executable(blink blink.cpp)
target_link_libraries(blink PRIVATE co1)

add_executable(echo_server echo_server.cpp)
target_link_libraries(echo_server PRIVATE co1)