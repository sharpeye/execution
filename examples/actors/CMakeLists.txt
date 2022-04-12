cmake_minimum_required(VERSION 3.20)

add_executable(actors
    main.cpp
    system.cpp
)

set_target_properties(actors PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(actors PRIVATE
    execution
    pthread
)
