#pragma once

#define TSB_DISALLOW_COPY_AND_ASSIGN(ClassName) \
  ClassName(const ClassName&) = delete;         \
  ClassName& operator=(const ClassName&) = delete;

#define TSB_DISALLOW_MOVE(ClassName)     \
  ClassName(const ClassName&&) = delete; \
  ClassName&& operator=(const ClassName&&) = delete;

#define TSB_LIKELY(condition) __builtin_expect(!!(condition), 1)
#define TSB_UNLIKELY(condition) __builtin_expect(!!(condition), 0)
