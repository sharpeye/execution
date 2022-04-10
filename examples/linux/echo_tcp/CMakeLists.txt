cmake_minimum_required(VERSION 3.20)

add_executable(echo_tcp main.cpp)

set_target_properties(echo_tcp PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(echo_tcp PRIVATE
    execution
    pthread
    liburing
)
