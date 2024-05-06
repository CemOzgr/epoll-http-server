cmake_minimum_required(VERSION 3.10)
project(MultiThreadedServer)

set(CMAKE_C_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(MultiThreadedServer 
    src/main.c
    src/server.c
    src/signaling.c
    src/task_queue.c 
    src/thread_pool.c
    src/client_table.c
    src/http.c
)