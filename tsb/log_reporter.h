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

  TSB_DISALLOW_COPY_AND_ASSIGN(LogReporter);
  TSB_DISALLOW_MOVE(LogReporter);

 private:
  std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace tsb
