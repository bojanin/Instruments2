if(APPLE)
  cmake_minimum_required(VERSION 4.0.2)
else()
  cmake_minimum_required(VERSION 3.22)
endif()
project(Bandicoot)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Paths
set(BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)
file(MAKE_DIRECTORY ${BUILD_DIR})
set(CMAKE_OSX_ARCHITECTURES
    arm64
    CACHE STRING "" FORCE)

add_subdirectory(captain_hook) 

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
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)

# Main app sources
set(SOURCES main.cc ${IMGUI_SOURCES})

# Executable
add_executable(bandicoot ${SOURCES})

target_include_directories(
  bandicoot PRIVATE program_state/ ${IMGUI_DIR} ${IMGUI_DIR}/backends
                    ${SDL2_INCLUDE_DIRS})
target_include_directories(bandicoot PRIVATE ${OPENGL_INCLUDE_DIR})

target_link_libraries(bandicoot PRIVATE ${SDL2_LIBRARIES} OpenGL::GL)

# Build flags
target_compile_options(
  bandicoot
  PRIVATE -g
          -O3
          -Wall
          -Wextra
          -Wpedantic
          -Werror
          -Wshadow
          -Wnon-virtual-dtor
          -Wcast-align
          -Wunused
          -Woverloaded-virtual
          -Wconversion
          -Wsign-conversion
          -Wnull-dereference
          -Wdouble-promotion
          -Wformat=2
          -Wimplicit-fallthrough
          -march=native)

set_target_properties(bandicoot PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                           ${BUILD_DIR})

add_custom_target(
  run
  COMMAND ${CMAKE_COMMAND} -E env ${BUILD_DIR}/bandicoot
  DEPENDS bandicoot
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
