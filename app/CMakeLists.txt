cmake_minimum_required(VERSION 3.20)

add_executable(example
    main.cpp
)

set_target_properties(example PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(example execution)
