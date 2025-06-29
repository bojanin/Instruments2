if(APPLE)
  cmake_minimum_required(VERSION 4.0.2)
else()
  cmake_minimum_required(VERSION 3.22)
endif()
project(instruments2)

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
add_subdirectory(tsb) 
add_subdirectory(pbtypes) 

# External packages
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(spdlog REQUIRED)
find_package(grpc CONFIG REQUIRED)

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
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp)

# Main app sources
set(SOURCES main.cc ${IMGUI_SOURCES})

SET(BASEPATH "${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${BASEPATH}")

# Executable
add_executable(instruments2 ${SOURCES})

target_include_directories(
  instruments2 PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends
                    ${SDL2_INCLUDE_DIRS})
target_include_directories(instruments2 PRIVATE ${OPENGL_INCLUDE_DIR})

target_include_directories(instruments2 SYSTEM PRIVATE ${PROJECT_SOURCE_DIR})

target_link_libraries(instruments2 PRIVATE ${SDL2_LIBRARIES} OpenGL::GL)
# https://github.com/gabime/spdlog/blob/v1.x/example/CMakeLists.txt
target_link_libraries(instruments2 PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)
target_link_libraries(instruments2 PUBLIC tsb gRPC::grpc++ pbtypes)


# Build flags
target_compile_options(
  instruments2
  PRIVATE -g
          -gcolumn-info
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

set_target_properties(instruments2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                           ${BUILD_DIR})
