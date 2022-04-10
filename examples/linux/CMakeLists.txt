cmake_minimum_required(VERSION 3.20)

include(FetchContent)

FetchContent_Declare(
    liburing_cmake
    GIT_REPOSITORY https://github.com/sharpeye/liburing-cmake.git
    GIT_TAG main
)

FetchContent_MakeAvailable(liburing_cmake)

add_subdirectory(echo_tcp)
