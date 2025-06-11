#include <sanitizer/tsan_interface.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <tsb/log_reporter.h>

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

extern "C" void __tsan_on_report(void* report) {
  const char* description = nullptr;
  int count = 0, stack_count = 0, mop_count = 0, loc_count = 0;
  int mutex_count = 0, thread_count = 0, unique_tid_count = 0;
  void* sleep_trace[16] = {};

  __tsan_get_report_data(report, &description, &count, &stack_count, &mop_count,
                         &loc_count, &mutex_count, &thread_count,
                         &unique_tid_count, sleep_trace, 16);

  SPDLOG_INFO("TSan Report: {}", description);
  SPDLOG_INFO("  Thread count: {}", thread_count);

  for (int i = 0; i < thread_count; ++i) {
    int tid = 0, parent_tid = 0, running = 0;
    uint64_t os_id = 0;
    const char* name = nullptr;
    void* trace[64] = {0};  // max depth

    __tsan_get_report_thread(report, i, &tid, &os_id, &running, &name,
                             &parent_tid, trace, 64);

    SPDLOG_INFO(
        "  ── Thread #{}  (tid={}, os_id={}, running={}, parent_tid={}, "
        "name='{}')",
        i, tid, os_id, running, parent_tid, name ? name : "");

    for (int j = 0; j < 64 && trace[j]; ++j) {
      SPDLOG_INFO("     {:>2}: PC = {:#018x}", j,
                  reinterpret_cast<uintptr_t>(trace[j]));
    }
  } /*
   const char* description;
   int count;
   int stack_count, mop_count, loc_count, mutex_count, thread_count,
       unique_tid_count;
   void* sleep_trace[16] = {0};
   __tsan_get_report_data(report, &description, &count, &stack_count,
   &mop_count, &loc_count, &mutex_count, &thread_count, &unique_tid_count,
   sleep_trace, 16); fprintf(stderr, "report type = '%s', count = %d\n",
   description, count); fprintf(stderr, "mop_count = %d\n", mop_count); int tid;
   void* addr;
   int size, write, atomic;
   void* trace[16] = {0};

   __tsan_get_report_mop(report, 0, &tid, &addr, &size, &write, &atomic, trace,
                         16);
   uint64_t os_id;
   int running;
   const char* name;
   int parent_tid;

   __tsan_get_report_thread(report, 0, &tid, &os_id, &running, &name,
                            &parent_tid, trace, 16);
   fprintf(stderr, "tid = %d\n", tid);
   // CHECK: tid = 1

   __tsan_get_report_thread(report, 1, &tid, &os_id, &running, &name,
                            &parent_tid, trace, 16);
   fprintf(stderr, "tid = %d\n", tid);
   */
}
