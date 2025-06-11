#pragma once
#include <cstdint>

namespace captain_hook {
constexpr const char* kServerPortEnvVar = "BANDICOOT_SERVER_PORT";
constexpr const char* kExecutablePathArgV0 = "BANDICOOT_SUBPROCESS_ARGV0";
constexpr const int32_t kDefaultServerPort = 50075;

}  // namespace captain_hook
