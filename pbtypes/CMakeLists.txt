find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf REQUIRED)

add_library(pbtypes instruments2.proto)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(TARGET pbtypes PROTOC_OUT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
protobuf_generate(
    TARGET pbtypes
    LANGUAGE grpc
    PROTOC_OUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    PLUGIN protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc)
