cmake_minimum_required(VERSION 3.20)

add_library(execution STATIC)

set_target_properties(execution PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(execution PUBLIC include)

target_sources(execution
    PRIVATE
    source/thread_pool.cpp
    source/timed_thread_pool.cpp
)
