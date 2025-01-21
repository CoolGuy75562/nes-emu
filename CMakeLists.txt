cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_BUILD TYPE Debug)

project (nes-emu)
add_subdirectory(src)
add_subdirectory(tests)
