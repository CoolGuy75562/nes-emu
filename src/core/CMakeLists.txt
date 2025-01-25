add_library(core STATIC
    cpu.c
    ppu.c
    memory.c
    cppwrapper.cpp
)
target_include_directories( core PUBLIC ${PROJECT_SOURCE_DIR}/include )

if(DEFINED HARTE_TESTS_PATH) 
    add_library(core_harte STATIC
        cpu.c
        ppu.c
        memory.c
        cppwrapper.cpp
    )
    target_include_directories( core_harte PUBLIC ${PROJECT_SOURCE_DIR}/include )
    target_compile_definitions(core_harte
        PRIVATE -DDOING_HARTE_TESTS=1
    )
endif()
