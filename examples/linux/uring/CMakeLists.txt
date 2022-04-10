cmake_minimum_required(VERSION 3.20)

add_library(uring INTERFACE)

set_target_properties(uring PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(uring INTERFACE include)

target_link_libraries(uring INTERFACE
    execution
    liburing
)
