find_package(Boost 1.65 REQUIRED COMPONENTS unit_test_framework)

# Copy over files required for core_tests
file(COPY
    ${PROJECT_SOURCE_DIR}/tests/nestest.nes
        DESTINATION ${PROJECT_BINARY_DIR}/tests
)

file(COPY
    ${PROJECT_SOURCE_DIR}/tests/nestest.log
        DESTINATION ${PROJECT_BINARY_DIR}/tests
)

file(COPY
    ${PROJECT_SOURCE_DIR}/tests/mapper_3.nes
        DESTINATION ${PROJECT_BINARY_DIR}/tests
)

# Build harte_tests if path given in harte_tests_dir_path.txt (see root CMakeLists)
# TODO: Some sanity checks here and in harte_tests.cpp
if(DEFINED HARTE_TESTS_PATH)
    ADD_CUSTOM_TARGET(link_target ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
    	    ${HARTE_TESTS_PATH}
            ${PROJECT_BINARY_DIR}/tests/harte_tests_dir
    )
    find_package(RapidJSON)
    add_library(harte harte.cpp)
    target_link_libraries(harte core_harte)
    add_executable(harte_tests harte_tests.cpp)
    target_link_libraries(harte_tests
        core_harte
	harte
	Boost::unit_test_framework
	#asan
    )
    target_compile_definitions(harte_tests
        PRIVATE
	    BOOST_TEST_DYN_LINK
    )
endif()

# build core_tests and require test to pass for build to succeed

add_library(nestest nestest.cpp)

target_link_libraries(nestest core)
#target_link_libraries(nestest asan)

add_executable(core_tests core_tests.cpp)

target_link_libraries(core_tests
    core
    nestest
    Boost::unit_test_framework
    #asan
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
