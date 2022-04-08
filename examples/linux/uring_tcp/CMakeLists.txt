cmake_minimum_required(VERSION 3.20)

include(ExternalProject)

ExternalProject_Add(
    liburing
    GIT_REPOSITORY https://github.com/axboe/liburing.git
    GIT_TAG liburing-2.1
    GIT_PROGRESS 1
    CONFIGURE_COMMAND ./configure
    BUILD_COMMAND make
    BUILD_IN_SOURCE 1
    BUILD_BYPRODUCTS "<SOURCE_DIR>/src/liburing.a"
    INSTALL_COMMAND ""
    TEST_COMMAND ""
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_MERGED_STDOUTERR ON
    LOG_OUTPUT_ON_FAILURE ON
)

ExternalProject_Get_Property(liburing SOURCE_DIR)

add_executable(uring_tcp
    main.cpp 
    # context.cpp
)

set_target_properties(uring_tcp PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(uring_tcp PRIVATE
    ${SOURCE_DIR}/src/include
)

target_link_libraries(uring_tcp PRIVATE
    execution
    pthread
    "${SOURCE_DIR}/src/liburing.a"
)
