add_compile_options(-Wall)
add_compile_options(-Werror)
add_compile_options(-pedantic)
add_compile_options(-Wextra)


find_package(CURL REQUIRED)
find_package(opentelemetry-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(re2 CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)



add_executable( test_api   Test.cpp)
target_link_libraries(
        test_api
        Metrics
        GTest::gtest_main
)
target_include_directories(test_api PUBLIC ${PROJECT_SOURCE_DIR}/src  ${OPENTELEMETRY_CPP_INCLUDE_DIRS})

