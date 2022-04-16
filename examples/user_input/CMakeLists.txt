cmake_minimum_required(VERSION 3.20)

add_executable(user_input
    main.cpp
)

set_target_properties(user_input PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(user_input PRIVATE
    execution
    pthread
)
