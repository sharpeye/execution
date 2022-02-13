cmake_minimum_required(VERSION 3.20)

add_library(execution INTERFACE)

set_target_properties(execution PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(execution INTERFACE include)
