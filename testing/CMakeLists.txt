include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable( test_api   Test.cpp)
target_link_libraries(
        test_api
        Metrics
        GTest::gtest_main
)
target_include_directories(test_api PUBLIC ${PROJECT_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src ${CURL_INCLUDE_DIRS} ${OPENTELEMETRY_CPP_INCLUDE_DIRS})

include(GoogleTest)
gtest_discover_tests(test_api)
