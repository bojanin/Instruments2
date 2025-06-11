#include <llvm/DebugInfo/DIContext.h>
#include <llvm/DebugInfo/Symbolize/Symbolize.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/raw_ostream.h>
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
static std::string GetExecutablePath() {
#if defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::string path(size, '\0');
  _NSGetExecutablePath(path.data(), &size);
  return path;
#elif defined(__linux__)
  char path[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
  return len > 0 ? std::string(path, len) : "";
#else
  return "<unsupported>";
#endif
}

std::string GetPc(void* pc, bool absolute) {
  Dl_info info{};
  if (!dladdr(pc, &info) || !info.dli_fname)
    return fmt::format("{} <no dladdr>", pc);

  const char* module_path = info.dli_fname;
  uint64_t address = reinterpret_cast<uintptr_t>(pc);

  llvm::symbolize::LLVMSymbolizer::Options opts;
  opts.PathStyle = llvm::DILineInfoSpecifier::FileLineInfoKind::BaseNameOnly;
  opts.UseSymbolTable = true;
  opts.Demangle = true;
  if (absolute) {
    opts.RelativeAddresses = false;
  } else {
    opts.RelativeAddresses = true;
    uint64_t module_base = reinterpret_cast<uintptr_t>(info.dli_fbase);
    address = reinterpret_cast<uintptr_t>(pc) - module_base;
    // TODO(bojanin): Explain this instead of YOLOing it.
    address += 0x100000000;
// TODO(bojanin): Explain this instead of YOLOing it.
#ifdef __aarch64__
    address -= 4;
#endif
  }
  // NOTE(bojanin): this doesn't need to be set if you're passing in DSYM paths
  llvm::symbolize::SectionedAddress addr{
      address, llvm::object::SectionedAddress::UndefSection};

  llvm::symbolize::LLVMSymbolizer sym(opts);
  llvm::Expected<llvm::DILineInfo> expected =
      sym.symbolizeCode(info.dli_fname, addr);
  SPDLOG_INFO("Symbolizing address:{}", (void*)addr.Address);

  if (!expected)
    return fmt::format("{} <symbolize error: {}>", pc,
                       llvm::toString(expected.takeError()));

  const llvm::DILineInfo& frame = *expected;
  if (frame.FileName.empty())
    return fmt::format("{} <unknown>", (void*)address);

  return fmt::format("{}:({}:{})", frame.FunctionName, frame.FileName,
                     frame.Line);
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
        SPDLOG_INFO("module base: {}@{} found symbol: {}@{}", info.dli_fname,
                    info.dli_fbase, info.dli_sname, info.dli_saddr);
        SPDLOG_INFO("LLVM: {}", GetPc(trace[f], false));
        if (info.dli_fname &&
            strstr(info.dli_fname, "libclang_rt.tsan") == nullptr) {
          // This is a user frame – print it the same way TSan does

          printf("  #%d %s+0x%lx\n", f, info.dli_fname,
                 (char*)trace[f] - (char*)info.dli_fbase);
        }
      }
    }
  }
}
