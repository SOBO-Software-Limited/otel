find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

add_compile_options(-Wall)
#add_compile_options(-Werror)
add_compile_options(-pedantic)
add_compile_options(-Wextra)


find_package(CURL REQUIRED)
find_package(opentelemetry-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(re2 CONFIG REQUIRED)


add_compile_options(-DWITH_STL=ON -Wno-unused-parameter)

link_directories(${CMAKE_BINARY_DIR}/lib)

add_compile_options(-DWITH_STL=ON)

add_executable(client client.cpp)
target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(client PUBLIC Metrics )


add_executable(server server.cpp)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(server PUBLIC Metrics )
