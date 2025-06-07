#include <stdio.h>
#include <stdlib.h>

#include "sanitizer/tsan_interface.h"  // optional but safe
extern "C" void __tsan_on_report(void* report) {
  printf("\n\n\n\n\n\n\n\nasdf\n\n\n\n\n\n");
  fprintf(stderr, "[rtlib] >>> __tsan_on_report() intercepted!\n");
}
extern "C" void __sanitizer_report_error_summary(const char* summary) {
  /* forward to GUI / IPC – for now just log */
  fprintf(stderr, "[rtlib] SUMMARY HOOK: %s\n", summary);
}
extern "C" void __sanitizer_on_print(const char* msg, size_t len) {
  /* very noisy: filter only ThreadSanitizer lines */
  fwrite(msg, 1, len, stderr); /* or send over IPC */

  /* example: detect end of report */
  fprintf(stderr, "[rtlib] --- end of TSan report ---\n");
}
