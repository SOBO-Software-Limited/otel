cmake_minimum_required(VERSION 3.19)
project(otel)

set (CMAKE_CXX_STANDARD 20)

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
add_subdirectory(trace)


#add_subdirectory(testing)


message(STATUS "We are on a ${CMAKE_SYSTEM_NAME} system")




