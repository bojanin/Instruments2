#include <stdio.h>

#include "sanitizer/tsan_interface.h"  // optional but safe

void __tsan_on_report(void* report) {
  printf("asdf");
  fprintf(stderr, "[rtlib] >>> __tsan_on_report() intercepted!\n");
}
