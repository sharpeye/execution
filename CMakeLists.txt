cmake_minimum_required(VERSION 3.20)

project(
    execution
    VERSION 0.1
    LANGUAGES CXX
)

add_subdirectory(lib)

# tests

include(CTest)
# if(BUILD_TESTING)

    include(FetchContent)

    FetchContent_Declare(
    gtest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(gtest)


    # include(gtest)
    add_subdirectory(test)
# endif(BUILD_TESTING)
