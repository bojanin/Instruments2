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

#include <tsb/dispatch_queue.h>
#include <tsb/server.h>

#include <print>

static char gSymbolicationScratchPad[8196];
static tsb::DispatchQueue& GetQueue() {
  static tsb::DispatchQueue q;
  return q;
}

static captain_hook::IPCClient& GetClient() {
  static captain_hook::IPCClient c;
  return c;
}

__attribute__((constructor)) void Init() {
  tsb::LogReporter::Create();
  // Spin up threads in contructor because tsan will deadlock if you do it
  // before.
  (void)GetClient();
  GetQueue().Dispatch([]() {});
  SPDLOG_WARN("Init Complete");
}

__attribute__((destructor)) void Deinit() {
  SPDLOG_INFO("Deinit Started");
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
std::string TsanSymbolizePC(void* pc) {
  std::memset(gSymbolicationScratchPad, 0, sizeof(gSymbolicationScratchPad));

  __sanitizer_symbolize_pc(pc, "%n %f %L%M", gSymbolicationScratchPad,
                           sizeof(gSymbolicationScratchPad));
  // TODO(bojanin): Extract out each item.
  // std::memset(gSymbolicationScratchPad, 0, sizeof(gSymbolicationScratchPad));

  return std::string(gSymbolicationScratchPad);
}

static inline std::string TsanSymbolizeMOP(void* addr) {
  char buf[8192];  // enough for one inline frame

  __sanitizer_symbolize_global(
      addr,
      "%n %S %L%M '%c'",  // same placeholders TSan uses
      buf, sizeof(buf));

  return std::string(buf);
}

// Prints: "Location is stack of thread 0."  (or heap / global / etc.)
static void PrintLocationBlock(void* report, unsigned long mop_index) {
  const char* loc_type = nullptr;
  void* loc_addr = nullptr;
  void* loc_start = nullptr;
  unsigned long loc_size = 0;
  int loc_tid = 0;
  int loc_fd = 0;
  int suppressable = 0;
  void* trace_loc[64] = {};

  // Returns 1 on success with the *new* prototype you pasted.
  if (__tsan_get_report_loc(report, mop_index, &loc_type, &loc_addr, &loc_start,
                            &loc_size, &loc_tid, &loc_fd, &suppressable,
                            trace_loc, 64) == 1 &&
      loc_type) {
    if (strcmp(loc_type, "stack") == 1) {
      SPDLOG_INFO("  Location is stack of thread {}.", loc_tid);
    } else if (strcmp(loc_type, "heap") == 0) {
      SPDLOG_INFO("  Location is heap block allocated by thread {}.", loc_tid);
    } else if (strcmp(loc_type, "global") == 0) {
      SPDLOG_INFO("  Location is global @ {:#x}.",
                  reinterpret_cast<uintptr_t>(loc_addr));
    } else {
      SPDLOG_INFO("  Location is {} of thread {}.", loc_type, loc_tid);
    }
    SPDLOG_INFO("");  // blank line just like TSan
  }
}

// RULES:
// Do not spawn fucking threads in here!!! TSAN will DEADLOCK.
extern "C" void __tsan_on_report(void* report) {
  const char* desc = nullptr;
  int count = 0, stack_cnt = 0, mop_cnt = 0, loc_cnt = 0;
  int mutex_cnt = 0, thr_cnt = 0, uniq_tid_cnt = 0;
  void* sleep_trace[16] = {};

  __tsan_get_report_data(report, &desc, &count, &stack_cnt, &mop_cnt, &loc_cnt,
                         &mutex_cnt, &thr_cnt, &uniq_tid_cnt, sleep_trace, 16);

  SPDLOG_INFO("══════════ TSan report: {}", desc ? desc : "<nullptr>");
  SPDLOG_INFO(
      "threads={}  mops={}  stacks={} duplicates={} loc_cnt={} mutex_cnt={} "
      "uniq={}",
      thr_cnt, mop_cnt, stack_cnt, count, loc_cnt, mutex_cnt, uniq_tid_cnt);

  for (int m = 0; m < mop_cnt; ++m) {
    int tid = 0, size = 0, is_write = 0, is_atomic = 0;
    void* addr = nullptr;
    void* trace[64] = {};
    __tsan_get_report_mop(report, m, &tid, &addr, &size, &is_write, &is_atomic,
                          trace, 64);

    SPDLOG_INFO("  {} of size {} at {:#018x} by {} thread:",
                is_write ? "Write" : "Read", size,
                reinterpret_cast<std::uintptr_t>(addr),
                tid == 0 ? "main" : fmt::format("thread T{}", tid));

    int frame_no = 0;
    for (int f = 0; trace[f]; ++f) {
      std::string line = TsanSymbolizePC(trace[f]);
      SPDLOG_INFO("    #{:<2} {}", frame_no++, line);
    }

    SPDLOG_INFO("END");

    // PrintLocationBlock(report, m);  // ← now uses the new API
  }

  GetQueue().Dispatch([]() {
    ::grpc::ClientContext context;
    static int x = 0;
    ::bandicoot::TestMsg request;
    request.set_first(fmt::format("hello: {}", x++));
    request.set_second("world");
    ::bandicoot::Void response;
    ::grpc::Status status =
        GetClient().stub_->OnSanitizerReport(&context, request, &response);
  });
}
