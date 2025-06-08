# ---------------------------------------------------------------------------
add_library(captain_hook SHARED tsan_ipc.cc)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Locate clang’s resource directory (cache so sub-dirs can see it)
execute_process(
  COMMAND clang -print-resource-dir
  OUTPUT_VARIABLE CLANG_RESOURCE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CLANG_RESOURCE_DIR "${CLANG_RESOURCE_DIR}" CACHE INTERNAL "clang resource dir")


# Headers treated as system
target_include_directories(captain_hook SYSTEM PUBLIC
    ${CLANG_RESOURCE_DIR}/include)
# Compile flags
target_compile_options(captain_hook PRIVATE -g -O3 -fPIC)
target_link_options(captain_hook PRIVATE -fsanitize=thread)

set_target_properties(captain_hook PROPERTIES
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/captain_hook
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
    POSITION_INDEPENDENT_CODE ON
    INSTALL_RPATH "@loader_path"
    BUILD_RPATH   "@loader_path")   # default suffix .dylib on macOS
