#include <spdlog/spdlog.h>
#include <tsb/dispatch_queue.h>

#include <print>
#include <utility>

namespace tsb {

DispatchQueue::~DispatchQueue() {  // graceful shutdown
  std::println("Deinit: DispatchQueue");
  {
    std::lock_guard lk(m_);
    stop_ = true;
  }
  cv_.notify_one();
  if (th_.joinable()) th_.join();
}

void DispatchQueue::Start() {
  std::call_once(started_,
                 [&] { th_ = std::thread(&DispatchQueue::Worker, this); });
}

void DispatchQueue::Dispatch(WorkItem w) {
  Start();  // start exactly once, and not sooner
  {
    std::lock_guard lk(m_);
    q_.push(std::move(w));
  }
  cv_.notify_one();
}

void DispatchQueue::Worker() {
  pthread_setname_np("tsb-dispatch-queue");
  for (;;) {
    WorkItem job;
    {
      std::unique_lock lk(m_);
      cv_.wait(lk, [&] { return stop_ || !q_.empty(); });
      if (stop_ && q_.empty()) return;
      job = std::move(q_.front());
      q_.pop();
    }
    job();  // run outside the lock
  }
}

}  // namespace tsb
