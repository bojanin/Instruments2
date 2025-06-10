add_library(tsb 
    STATIC
    log_reporter.cc
)

SET(BASEPATH "${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${BASEPATH}")
find_package(spdlog REQUIRED)
target_link_libraries(tsb PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

