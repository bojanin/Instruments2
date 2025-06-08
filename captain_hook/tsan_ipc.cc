#include <sanitizer/tsan_interface.h>
#include <stdio.h>

#include <print>
__attribute__((constructor)) void Init() {
  //
}

__attribute__((destructor)) void Deinit() {
  //
}

extern "C" void __tsan_on_report(void* report) {
  const char* description;
  int count;
  int stack_count, mop_count, loc_count, mutex_count, thread_count,
      unique_tid_count;
  void* sleep_trace[16] = {0};
  __tsan_get_report_data(report, &description, &count, &stack_count, &mop_count,
                         &loc_count, &mutex_count, &thread_count,
                         &unique_tid_count, sleep_trace, 16);
  fprintf(stderr, "report type = '%s', count = %d\n", description, count);
  fprintf(stderr, "mop_count = %d\n", mop_count);
  int tid;
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
}
