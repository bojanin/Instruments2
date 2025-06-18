#pragma once
#include <cstdint>

namespace tsb {
constexpr const char* kServerPortEnvVar = "BANDICOOT_SERVER_PORT";
constexpr const char* kExecutablePathArgV0 = "BANDICOOT_SUBPROCESS_ARGV0";
constexpr const int32_t kDefaultServerPort = 50075;
constexpr int TRACE_MAX = 128;  // max PCs we keep per stack
constexpr int ARRAY_MAX = 64;   // max objects per array in the proto

}  // namespace tsb
