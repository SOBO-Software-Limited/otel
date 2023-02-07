cmake_minimum_required(VERSION 3.19)
project(otel)

set (CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

add_compile_options(-Wall)
#add_compile_options(-Werror)
add_compile_options(-pedantic)
add_compile_options(-Wextra)
add_compile_options(-std=c++20)

find_package(CURL REQUIRED)
find_package(opentelemetry-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(re2 CONFIG REQUIRED)
find_package(Boost COMPONENTS filesystem system unit_test_framework program_options REQUIRED)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-DWITH_STL=ON -Wno-unused-parameter)

include_directories(. src ${OPENTELEMETRY_CPP_INCLUDE_DIRS})

add_subdirectory(src/libMetrics)


enable_testing()

add_executable( test_api   ${PROJECT_SOURCE_DIR}/testing/Test.cpp)
target_link_libraries(
        test_api
        Metrics
        GTest::gtest_main
)
target_include_directories(test_api PUBLIC ${PROJECT_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src ${CURL_INCLUDE_DIRS} ${OPENTELEMETRY_CPP_INCLUDE_DIRS})

include(GoogleTest)
gtest_discover_tests(test_api)



message(STATUS "We are on a ${CMAKE_SYSTEM_NAME} system")




