#include <dlfcn.h>

#include <cstdio>

#include "tsan_interface.h"

extern "C" void __tsan_on_report(void* report) {
  // Custom behavior on TSan report
  // For example, print the report or send it to main_program
  printf("TSB TSB: Intercepted TSan report!\n");

  // You can access report details via `rep` and use IPC to send it back to
  // main_program if needed

  // Optional: call the original function if needed
  // Uncomment the following lines if you want to invoke the default behavior
  /*
  typedef void (*TsanOnReportFunc)(const ReportDesc *);
  static TsanOnReportFunc orig_tsan_on_report =
  (TsanOnReportFunc)dlsym(RTLD_NEXT, "__tsan_on_report"); if
  (orig_tsan_on_report) { orig_tsan_on_report(rep);
  }
  */
}
