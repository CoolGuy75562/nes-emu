set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets OpenGL)

add_executable(nes-emu
    main.cpp
    mainwindow.ui
    mainwindow.cpp
    nescontext.cpp
    nesscreen.cpp
    nescontroller.cpp
    openglwidget.cpp
)

target_include_directories(nes-emu PRIVATE ${PROJECT_SOURCE_DIR}/src/app )

set_target_properties(nes-emu
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
)

target_link_libraries(nes-emu
    core
    Qt5::Widgets
    Qt5::OpenGL
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARY}
)
