add_executable(nes-emu main.cpp)
set_target_properties(nes-emu
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
)

target_link_libraries(nes-emu core)
