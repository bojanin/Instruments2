#pragma once
#include <vector>

namespace captain_hook {
enum class SanitizerType {
  UNKNOWN = 0,
  TSAN = 1,
  ASAN = 2,
  UBSAN = 3,
};

struct UbsanMessage {};
struct AsanMessage {};
struct TsanMessage {};

union SanitizerMessage {
  UbsanMessage ubsan;
  AsanMessage asan;
  TsanMessage tsan;
};

struct MessageHeader {
  const uint32_t magic_header = 0xBEEFB00B;
  SanitizerType type;
};

struct CaptainHookMessage {
  MessageHeader header;
  SanitizerMessage message;
};

// Helpers
static CaptainHookMessage Deserialize(std::vector<std::byte> data);
static std::vector<std::byte> Serialize(const CaptainHookMessage& msg);

}  // namespace captain_hook
