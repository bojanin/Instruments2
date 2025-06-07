cmake_minimum_required(VERSION 4.0.2)
project(Bandicoot)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Paths
set(BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)
file(MAKE_DIRECTORY ${BUILD_DIR})

# LLVM & compiler-rt setup
set(LLVM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/llvm-project/llvm)
set(LLVM_BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/llvm)
set(COMPILER_RT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/llvm-project/compiler-rt)

set(LLVM_ENABLE_RUNTIMES "compiler-rt" CACHE STRING "" FORCE)
set(LLVM_ENABLE_PROJECTS  "clang"          CACHE STRING "" FORCE)
set(LLVM_RUNTIME_TARGETS "host" CACHE STRING "" FORCE)
set(LLVM_TARGETS_TO_BUILD "AArch64" CACHE STRING "" FORCE)
set(LLVM_INCLUDE_TESTS OFF CACHE BOOL "" FORCE)
set(COMPILER_RT_INCLUDE_TESTS OFF CACHE BOOL "" FORCE)

# Explicitly turn on sanitizers (TSAN included)
set(COMPILER_RT_BUILD_SANITIZERS ON CACHE BOOL "" FORCE)
set(COMPILER_RT_ENABLE_TSAN ON CACHE BOOL "" FORCE)

# Cross-arch settings (just host for now)
set(LLVM_TARGETS_TO_BUILD "AArch64" CACHE STRING "" FORCE)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "" FORCE)

add_subdirectory(${LLVM_SOURCE_DIR} ${LLVM_BINARY_DIR} EXCLUDE_FROM_ALL)
add_subdirectory(librt)  # Add shared lib subproject

# SDL2
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# ImGui
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

# Main app sources
set(SOURCES
    main.cc
    ${IMGUI_SOURCES}
    program_state/state.cc
)

# Executable
add_executable(bandicoot ${SOURCES})

target_include_directories(bandicoot PRIVATE
    program_state/
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
)
target_include_directories(bandicoot PRIVATE ${OPENGL_INCLUDE_DIR})


target_link_libraries(bandicoot PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
)

# Build flags
target_compile_options(bandicoot PRIVATE 
    -g -O3 -Wall -Werror -Wextra -Wshadow -Wnon-virtual-dtor -Wcast-align
    -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion
    -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough
    -march=native
)

set_target_properties(bandicoot PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR}
)

add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E env ${BUILD_DIR}/bandicoot
    DEPENDS bandicoot
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
