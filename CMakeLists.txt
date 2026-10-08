cmake_minimum_required(VERSION 3.10)
project(lowlatency_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server
    src/main.cpp
    include/server/event_loop.cpp
    include/server/acceptor.cpp
    include/server/tcpserver.cpp
    include/server/eventloopthread.cpp
    include/server/thread_pool.cpp
    include/server/channel.cpp
    include/server/tcp_connnection.cpp
    include/server/buffer.cpp
    include/server/length_header_codec.cpp
    include/server/timer_queue.cpp
    include/server/socket.cpp
    include/server/log.cpp
    include/server/errno.cpp
)

target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 独立定时器演示（不污染 server 的 echo 业务）
add_executable(timer_demo
    src/timer_demo.cpp
    include/server/event_loop.cpp
    include/server/channel.cpp
    include/server/timer_queue.cpp
    include/server/socket.cpp
    include/server/log.cpp
    include/server/errno.cpp
)
target_include_directories(timer_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)