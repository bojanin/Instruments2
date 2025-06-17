#pragma once
#include <tsb/macros.h>

#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace tsb {
class DispatchQueue {
  using WorkItem = std::function<void()>;

 public:
  DispatchQueue() = default;
  ~DispatchQueue();

  /// Enqueue work. Starts the worker lazily and is 100 % thread-safe.
  void Dispatch(WorkItem w);

  TSB_DISALLOW_COPY_AND_ASSIGN(DispatchQueue);
  TSB_DISALLOW_MOVE(DispatchQueue);

 private:
  void Start();
  void Worker();

  std::mutex m_;
  std::condition_variable cv_;
  std::queue<WorkItem> q_;
  std::atomic<bool> stop_{false};
  std::once_flag started_;
  std::thread th_;
};
}  // namespace tsb
