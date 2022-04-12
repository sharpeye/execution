cmake_minimum_required(VERSION 3.20)

add_subdirectory(actors)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory(linux)
endif()
