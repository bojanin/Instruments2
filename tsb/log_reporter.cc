#include "tsb/log_reporter.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace tsb {

void LogReporter::Create() { (void)LogReporter::Shared(); }

std::shared_ptr<LogReporter> LogReporter::Shared() {
  static std::shared_ptr<LogReporter> reporter;
  if (TSB_LIKELY(reporter)) {
    return reporter;
  }
  reporter = std::make_shared<LogReporter>();
  return reporter;
}

LogReporter::LogReporter() {
  auto console = spdlog::stdout_color_mt("Bandicoot");
  console->set_pattern("[%L] [%DT%H:%M:%S.%f]%^ [%s:%#] %v%$");
  console->enable_backtrace(64);
  spdlog::set_default_logger(console);
}

}  // namespace tsb
