#include <sanitizer/common_interface_defs.h>
#include <sanitizer/tsan_interface.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <tsb/log_reporter.h>

#ifdef __APPLE__
#include <dlfcn.h>
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

#include <print>

#include "captain_hook/server.h"

std::thread gServerThread;

__attribute__((constructor)) void Init() {
  tsb::LogReporter::Create();
  SPDLOG_INFO("Init");
  captain_hook::IPCServer::Create();
  gServerThread =
      std::thread([]() { captain_hook::IPCServer::Shared()->RunForever(); });
  SPDLOG_WARN("Init Complete");
}

__attribute__((destructor)) void Deinit() {
  SPDLOG_INFO("Deinit Started");
  captain_hook::IPCServer::Shared()->SetExitFlag(true);
  if (gServerThread.joinable()) {
    gServerThread.join();
  }
  SPDLOG_INFO("Deinit Complete");
  //
}

// Here's the full list of available placeholders:
//   %% - represents a '%' character;
//   %n - frame number (copy of frame_no);
//   %p - PC in hex format;
//   %m - path to module (binary or shared object);
//   %o - offset in the module in hex format;
//   %f - function name;
//   %q - offset in the function in hex format (*if available*);
//   %s - path to source file;
//   %l - line in the source file;
//   %c - column in the source file;
//   %F - if function is known to be <foo>, prints "in <foo>", possibly
//        followed by the offset in this function, but only if source file
//        is unknown;
//   %S - prints file/line/column information;
//   %L - prints location information: file/line/column, if it is known, or
//        module+offset if it is known, or (<unknown module>) string.
//   %M - prints module basename and offset, if it is known, or PC.
static inline std::string TsanSymbolizePC(void* pc) {
  char buf[1024];  // enough for one inline frame

  // The sanitizer API returns one C-string per inline frame,
  // terminated by an empty string. We loop until that empty one.
  __sanitizer_symbolize_pc(pc,
                           "%n %L%M",  // same placeholders TSan uses
                           buf, sizeof(buf));

  return std::string(buf);
}

static inline std::string TsanSymbolizeMOP(void* addr) {
  char buf[1024];  // enough for one inline frame

  __sanitizer_symbolize_global(addr,
                               "%n %S %L%M",  // same placeholders TSan uses
                               buf, sizeof(buf));

  return std::string(buf);
}

extern "C" void __tsan_on_report(void* report) {
  const char* desc = nullptr;
  int count = 0, stack_cnt = 0, mop_cnt = 0, loc_cnt = 0;
  int mutex_cnt = 0, thr_cnt = 0, uniq_tid_cnt = 0;
  void* sleep_trace[16] = {};

  __tsan_get_report_data(report, &desc, &count, &stack_cnt, &mop_cnt, &loc_cnt,
                         &mutex_cnt, &thr_cnt, &uniq_tid_cnt, sleep_trace, 16);

  SPDLOG_INFO("══════════ TSan report: {}", desc ? desc : "<nullptr>");
  SPDLOG_INFO("threads={}  mops={}  stacks={}", thr_cnt, mop_cnt, stack_cnt);

  // Iterate over every MOP (read/write) that participates in the race
  for (int m = 0; m < mop_cnt; ++m) {
    int mop_tid = 0, size = 0, is_write = 0, is_atomic = 0;
    void* addr = nullptr;
    void* trace[64] = {};

    __tsan_get_report_mop(report, m, &mop_tid, &addr, &size, &is_write,
                          &is_atomic, trace, 64);

    SPDLOG_INFO("INFO: {}", TsanSymbolizeMOP(addr));
    SPDLOG_INFO("─── MOP #{} on addr {:#018x} (tid={}, {}{}{})", m,
                reinterpret_cast<std::uintptr_t>(addr), mop_tid,
                is_write ? "write" : "read", is_atomic ? ", atomic" : "",
                size ? fmt::format(", size={}", size) : "");

    // Print backtrace, skipping frames in the TSan runtime itself
    for (int m = 0; m < mop_cnt; ++m) {
      int tid, size, is_write, is_atomic;
      void *addr, *trace[64] = {};
      __tsan_get_report_mop(report, m, &tid, &addr, &size, &is_write,
                            &is_atomic, trace, 64);

      for (int f = 0; f < 64 && trace[f]; ++f) {
        Dl_info info{};
        dladdr(trace[f], &info);
        SPDLOG_INFO("Race: {}", TsanSymbolizePC(trace[f]));
      }
    }
  }
}
