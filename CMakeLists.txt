cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (EXISTS ${CMAKE_SOURCE_DIR}/harte_tests_dir_path.txt) 
    file(STRINGS harte_tests_dir_path.txt HARTE_TESTS_PATH)
    message("harte_tests_dir_path.txt found:")
    message(${HARTE_TESTS_PATH})
else()
    message("harte_tests_dir_path.txt not found")
endif()

add_compile_options("-g")
add_compile_options("-Og")
add_compile_options("-Wall")
#add_compile_options("-fsanitize=address")

project (nes-emu)
add_subdirectory(src)
add_subdirectory(tests)
