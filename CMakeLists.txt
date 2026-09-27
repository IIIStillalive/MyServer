cmake_minimum_required(VERSION 3.10)
project(lowlatency_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server
    src/main.cpp
    include/server/event_loop.cpp
)

target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)