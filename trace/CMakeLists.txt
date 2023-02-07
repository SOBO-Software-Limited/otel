link_directories(${CMAKE_BINARY_DIR}/lib)

add_compile_options(-DWITH_STL=ON)

add_executable(client client.cpp)
target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(client PUBLIC Metrics Boost::program_options)


add_executable(server server.cpp)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(server PUBLIC Metrics Boost::program_options)
