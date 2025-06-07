project(librt)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

# librt shared library
add_library(librt SHARED
    report_impl.c
    # add other sources here
)

# Include sanitizer headers
# Add compiler-rt include paths
target_include_directories(librt PRIVATE
    ${COMPILER_RT_DIR}/include
    ${COMPILER_RT_DIR}/lib
    ${COMPILER_RT_DIR}/lib/tsan/rtl
)


# Set output dir
set_target_properties(librt PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/librt
)

# Mac-specific DYLD preload support flags (if needed)
target_link_options(librt PRIVATE
    -Wno-unused-command-line-argument
)
