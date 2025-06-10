#include "tsb/log_reporter.h"

namespace tsb {

void LogReporter::Create() { LogReporter::Shared()->SetupLogger(); }

std::shared_ptr<LogReporter> LogReporter::Shared() {
  static std::shared_ptr<LogReporter> reporter;
  if (TSB_LIKELY(reporter)) {
    return reporter;
  }
  reporter = std::make_shared<LogReporter>();
  return reporter;
}

LogReporter::LogReporter() {
  auto console = spdlog::stdout_color_st("Bandicoot");
  console->set_pattern("[%L] [%DT%H:%M:%S.%f]%^ [%s:%#] %v%$");
  console->enable_backtrace(64);
  spdlog::set_default_logger(console);
  spdlog::enable_backtrace(64);
  // spdlog::set_pattern("[%D %H:%M:%S.%f][%L:%t] %@: %v");
}

}  // namespace tsb
