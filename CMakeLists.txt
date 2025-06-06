cmake_minimum_required(VERSION 3.30)

# Project Name
project(Bandicoot)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define the build directory
set(BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)

# Find SDL2
find_package(SDL2 REQUIRED)

# Add ImGui source files
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Add your main source file
set(SOURCES
    main.cc
    ${IMGUI_SOURCES}
    program_state/state.cc
)

# Create the build directory if it doesn't exist
file(MAKE_DIRECTORY ${BUILD_DIR})

# Create the executable
add_executable(bandicoot ${SOURCES})

# Specify include directories
target_include_directories(bandicoot PRIVATE
    program_state/
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
)

# Link libraries (and frameworks for macOS)
if(APPLE)
    target_link_libraries(bandicoot PRIVATE
        ${SDL2_LIBRARIES}
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
else()
    target_link_libraries(bandicoot PRIVATE
        ${SDL2_LIBRARIES}
        OpenGL
    )
endif()

target_compile_options(bandicoot PRIVATE 
        -g
        -O3
        -Wall
        -Wno-error
        -Werror
        -Wextra # reasonable and standard
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps
        # catch hard to track down memory errors
        -Wcast-align # warn for potential performance problem casts
        -Wunused # warn on anything being unused
        -Woverloaded-virtual # warn if you overload (not override) a virtual function
        -Wpedantic # warn if non-standard C++ is used
        -Wconversion # warn on type conversions that may lose data
        -Wsign-conversion # warn on sign conversions
        -Wnull-dereference # warn if a null dereference is detected
        -Wdouble-promotion # warn if float is implicit promoted to double
        -Wformat=2 # warn on security issues around functions that format output (ie printf)
        -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
        -march=native
    )


# Set output directory for the executable
set_target_properties(bandicoot PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR}
)

# Optionally, add a custom command to run the executable
add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E env ${BUILD_DIR}/bandicoot
    DEPENDS bandicoot
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
