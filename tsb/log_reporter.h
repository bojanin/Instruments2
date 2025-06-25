#pragma once
#include <spdlog/spdlog.h>

#include <memory>

#include "tsb/macros.h"

namespace tsb {
class LogReporter {
 public:
  LogReporter();
  static std::shared_ptr<LogReporter> Shared();
  static void Create();

  // Creates a directory and new file in /tmp that will be used for monitoring
  // the current runs output.
  // If it fails it will returns the reason behind the failure.
  std::optional<std::string> PrepareForSanitizerOutput();

  bool SanitizerOutputExists() const { return false; }
  const std::string& SanitizerOutput() { return sanitizer_output_; }

  TSB_DISALLOW_COPY_AND_ASSIGN(LogReporter);
  TSB_DISALLOW_MOVE(LogReporter);

 private:
  std::string sanitizer_output_ = "";
  std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace tsb
