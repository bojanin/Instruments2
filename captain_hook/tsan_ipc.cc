#include <stdio.h>

#include <print>

#include "sanitizer/tsan_interface.h"  // optional but safe
__attribute__((constructor)) void Init() {
  //
}

__attribute__((destructor)) void Deinit() {
  //
}

extern "C" void __tsan_on_report(void* report) {
  __tsan_get_current_report();
  //
  std::print("Here\n");
}
