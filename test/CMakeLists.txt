file(GLOB test-sources "*_test.cpp")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    file(GLOB linux-test-sources "linux/*_test.cpp")
    list(APPEND test-sources ${linux-test-sources})
endif()

foreach(file-path ${test-sources})
    string( REPLACE ".cpp" "" file-path-without-ext ${file-path} )
    get_filename_component(file-name ${file-path-without-ext} NAME)
    add_executable( ${file-name} ${file-path})

    set_target_properties(${file-name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    message ("CLANG: ${CLANG_VERSION_STRING}")

    target_link_libraries(${file-name} PUBLIC gtest_main execution)
    add_test(NAME "test-${file-name}" COMMAND ${file-name})
endforeach()
