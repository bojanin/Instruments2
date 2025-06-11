# ---------------------------------------------------------------------------
add_library(captain_hook 
    SHARED
    tsan_ipc.cc
    server.cc
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


execute_process(
  COMMAND clang -print-resource-dir
  OUTPUT_VARIABLE CLANG_RESOURCE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CLANG_RESOURCE_DIR "${CLANG_RESOURCE_DIR}" CACHE INTERNAL "clang resource dir")
find_package(spdlog REQUIRED)


target_include_directories(captain_hook SYSTEM PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(captain_hook SYSTEM PUBLIC
    ${CLANG_RESOURCE_DIR}/include)

target_link_libraries(captain_hook PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)
target_link_libraries(captain_hook PRIVATE tsb)




# Options
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
