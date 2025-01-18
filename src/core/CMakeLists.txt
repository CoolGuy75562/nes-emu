add_library(core STATIC
    cpu.c
    ppu.c
    memory.c
    cppwrapper.cpp
)

target_include_directories( core PUBLIC ${PROJECT_SOURCE_DIR}/include )
