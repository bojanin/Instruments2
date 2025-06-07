#include <stdio.h>

#include "sanitizer/tsan_interface.h"  // optional but safe

extern "C" void _tsan_on_report(void* report) {
  printf("\n\n\n\n\n\n\n\nasdf\n\n\n\n\n\n");
  fprintf(stderr, "[rtlib] >>> __tsan_on_report() intercepted!\n");
}
extern "C" void _sanitizer_report_error_summary(const char* summary) {
  /* forward to GUI / IPC – for now just log */
  fprintf(stderr, "[rtlib] SUMMARY HOOK: %s\n", summary);
}
