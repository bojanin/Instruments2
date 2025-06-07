project(captain_hook)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

# captain_hook shared library
add_library(captain_hook SHARED
    tsan_ipc.cc
    # add other sources here
)

# Include sanitizer headers
# Add compiler-rt include paths
target_include_directories(captain_hook PRIVATE
    ${COMPILER_RT_DIR}/include
    ${COMPILER_RT_DIR}/lib
    ${COMPILER_RT_DIR}/lib/tsan/rtl
)

target_compile_options(captain_hook PRIVATE
    -std=c++23
    -g
    -fPIC
    -O3)

set_target_properties(captain_hook PROPERTIES SUFFIX ".so")

# Set output dir
set_target_properties(captain_hook PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/captain_hook

)
set_target_properties(captain_hook PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
    POSITION_INDEPENDENT_CODE ON
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/captain_hook
)
