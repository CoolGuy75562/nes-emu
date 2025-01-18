file(COPY
    ${PROJECT_SOURCE_DIR}/tests/nestest.nes
        DESTINATION ${PROJECT_BINARY_DIR}/tests
)

file(COPY
    ${PROJECT_SOURCE_DIR}/tests/nestest.log
        DESTINATION ${PROJECT_BINARY_DIR}/tests
)

file(COPY
    ${PROJECT_SOURCE_DIR}/tests/mapper_6.nes
        DESTINATION ${PROJECT_BINARY_DIR}/tests
)

find_package(Boost 1.46.1 REQUIRED COMPONENTS unit_test_framework)

add_library(nestest nestest.cpp)

target_link_libraries(nestest core)

# unit testing

add_executable(core_tests core_tests.cpp)

target_link_libraries(core_tests
    core
    nestest
    Boost::unit_test_framework
)

target_compile_definitions(core_tests
    PRIVATE
        BOOST_TEST_DYN_LINK
)

enable_testing()
add_test(core_tests core_tests)

add_custom_command(
    TARGET core_tests
    POST_BUILD
    COMMAND ctest --output-on-failure
)
