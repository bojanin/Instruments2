#include <sanitizer/common_interface_defs.h>
#include <sanitizer/tsan_interface.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <tsb/log_reporter.h>

#include <fstream>

#ifdef __APPLE__
#include <dlfcn.h>
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

#include <tsb/dispatch_queue.h>
#include <tsb/server.h>

#include <print>

static char gSymbolicationScratchPad[8196];
static tsb::DispatchQueue& GetQueue() {
  static tsb::DispatchQueue q;
  return q;
}

static captain_hook::IPCClient& GetClient() {
  static captain_hook::IPCClient c;
  return c;
}

__attribute__((constructor)) void Init() {
  tsb::LogReporter::Create();
  // Spin up threads in contructor because tsan will deadlock if you do it
  // before.
  (void)GetClient();
  GetQueue().Dispatch([]() {});
  SPDLOG_INFO("==TSAN SHLIB Init==");
  SPDLOG_INFO("using protobuf {}", google::protobuf::internal::VersionString(
                                       GOOGLE_PROTOBUF_VERSION));
  SPDLOG_INFO("==TSAN SHLIB End Init==");
}

__attribute__((destructor)) void Deinit() {
  SPDLOG_INFO("Deinit Started");
  SPDLOG_INFO("Deinit Complete");
  //
}
// Here's the full list of available placeholders:
//   %% - represents a '%' character;
//   %n - frame number (copy of frame_no);
//   %p - PC in hex format;
//   %m - path to module (binary or shared object);
//   %o - offset in the module in hex format;
//   %f - function name;
//   %q - offset in the function in hex format (*if available*);
//   %s - path to source file;
//   %l - line in the source file;
//   %c - column in the source file;
//   %F - if function is known to be <foo>, prints "in <foo>", possibly
//        followed by the offset in this function, but only if source file
//        is unknown;
//   %S - prints file/line/column information;
//   %L - prints location information: file/line/column, if it is known, or
//        module+offset if it is known, or (<unknown module>) string.
//   %M - prints module basename and offset, if it is known, or PC.
void XXXX() {
  const int REPORT_TRACE_SIZE = 128;
  const int REPORT_ARRAY_SIZE = 64;

  struct {
    void* report;
    const char* description;
    int report_count;

    void* sleep_trace[REPORT_TRACE_SIZE];

    int stack_count;
    struct {
      int idx;
      void* trace[REPORT_TRACE_SIZE];
    } stacks[REPORT_ARRAY_SIZE];

    int mop_count;
    struct {
      int idx;
      int tid;
      int size;
      int write;
      int atomic;
      void* addr;
      void* trace[REPORT_TRACE_SIZE];
    } mops[REPORT_ARRAY_SIZE];

    int loc_count;
    struct {
      int idx;
      const char* type;
      void* addr;
      void* start;
      unsigned long size;
      int tid;
      int fd;
      int suppressable;
      void* trace[REPORT_TRACE_SIZE];
    } locs[REPORT_ARRAY_SIZE];

    int mutex_count;
    struct {
      int idx;
      uint64_t mutex_id;
      void* addr;
      int destroyed;
      void* trace[REPORT_TRACE_SIZE];
    } mutexes[REPORT_ARRAY_SIZE];

    int thread_count;
    struct {
      int idx;
      int tid;
      uint64_t os_id;
      int running;
      const char* name;
      int parent_tid;
      void* trace[REPORT_TRACE_SIZE];
    } threads[REPORT_ARRAY_SIZE];

    int unique_tid_count;
    struct {
      int idx;
      int tid;
    } unique_tids[REPORT_ARRAY_SIZE];
  } t = {0};

  t.report = __tsan_get_current_report();
  __tsan_get_report_data(t.report, &t.description, &t.report_count,
                         &t.stack_count, &t.mop_count, &t.loc_count,
                         &t.mutex_count, &t.thread_count, &t.unique_tid_count,
                         t.sleep_trace, REPORT_TRACE_SIZE);

  if (t.stack_count > REPORT_ARRAY_SIZE) t.stack_count = REPORT_ARRAY_SIZE;
  for (int i = 0; i < t.stack_count; i++) {
    t.stacks[i].idx = i;
    __tsan_get_report_stack(t.report, i, t.stacks[i].trace, REPORT_TRACE_SIZE);
  }

  if (t.mop_count > REPORT_ARRAY_SIZE) t.mop_count = REPORT_ARRAY_SIZE;
  for (int i = 0; i < t.mop_count; i++) {
    t.mops[i].idx = i;
    __tsan_get_report_mop(t.report, i, &t.mops[i].tid, &t.mops[i].addr,
                          &t.mops[i].size, &t.mops[i].write, &t.mops[i].atomic,
                          t.mops[i].trace, REPORT_TRACE_SIZE);
  }

  if (t.loc_count > REPORT_ARRAY_SIZE) t.loc_count = REPORT_ARRAY_SIZE;
  for (int i = 0; i < t.loc_count; i++) {
    t.locs[i].idx = i;
    __tsan_get_report_loc(t.report, i, &t.locs[i].type, &t.locs[i].addr,
                          &t.locs[i].start, &t.locs[i].size, &t.locs[i].tid,
                          &t.locs[i].fd, &t.locs[i].suppressable,
                          t.locs[i].trace, REPORT_TRACE_SIZE);
  }

  if (t.mutex_count > REPORT_ARRAY_SIZE) t.mutex_count = REPORT_ARRAY_SIZE;
  for (int i = 0; i < t.mutex_count; i++) {
    t.mutexes[i].idx = i;
    __tsan_get_report_mutex(t.report, i, &t.mutexes[i].mutex_id,
                            &t.mutexes[i].addr, &t.mutexes[i].destroyed,
                            t.mutexes[i].trace, REPORT_TRACE_SIZE);
  }

  if (t.thread_count > REPORT_ARRAY_SIZE) t.thread_count = REPORT_ARRAY_SIZE;
  for (int i = 0; i < t.thread_count; i++) {
    t.threads[i].idx = i;
    __tsan_get_report_thread(t.report, i, &t.threads[i].tid,
                             &t.threads[i].os_id, &t.threads[i].running,
                             &t.threads[i].name, &t.threads[i].parent_tid,
                             t.threads[i].trace, REPORT_TRACE_SIZE);
  }

  if (t.unique_tid_count > REPORT_ARRAY_SIZE)
    t.unique_tid_count = REPORT_ARRAY_SIZE;
  for (int i = 0; i < t.unique_tid_count; i++) {
    t.unique_tids[i].idx = i;
    __tsan_get_report_unique_tid(t.report, i, &t.unique_tids[i].tid);
  }

  (void)t;
}
// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
constexpr int TRACE_MAX = 128;  // max PCs we keep per stack
constexpr int ARRAY_MAX = 64;   // max objects per array in the proto

// -----------------------------------------------------------------------------
//  PC → StackFrame
// -----------------------------------------------------------------------------
static bandicoot::StackFrame makeFrame(void* pc) {
  char buf[512] = {};
  // "%s:%l:%f"  →  "file.cc:42:func"
  __sanitizer_symbolize_pc(pc, "%s:%l:%f", buf, sizeof(buf));

  bandicoot::StackFrame f;
  f.set_repr(buf);

  const char* c1 = strchr(buf, ':');
  const char* c2 = c1 ? strchr(c1 + 1, ':') : nullptr;
  if (c1 && c2) {
    f.set_file_name({buf, static_cast<size_t>(c1 - buf)});
    f.set_line(static_cast<int64_t>(std::atoi(c1 + 1)));
    f.set_function(c2 + 1);
  }
  return f;
}

// Append user frames from |trace| into proto repeated<StackFrame>.
static void appendFrames(void** trace, bandicoot::Stack& dst) {
  for (int i = 0; trace[i] && i < TRACE_MAX; ++i) {
    std::string txt = makeFrame(trace[i]).repr();
    *dst.add_frames() = makeFrame(trace[i]);
  }
}

// -----------------------------------------------------------------------------
// Build a Stack from one MOP index & emit human text to |raw|
// -----------------------------------------------------------------------------
// static bandicoot::Stack buildStack(void* report, unsigned long mop_idx,
//                                   std::string& raw) {
//  int tid = 0, sz = 0, wr = 0, at = 0;
//  void* addr = nullptr;
//  void* trace[TRACE_MAX] = {};
//  __tsan_get_report_mop(report, mop_idx, &tid, &addr, &sz, &wr, &at, trace,
//                        TRACE_MAX);
//
//  const char* rw = wr ? "Write" : "Read";
//  std::string header =
//      fmt::format("  {} of size {} at {:#018x} by {} thread:", rw, sz,
//                  reinterpret_cast<std::uintptr_t>(addr),
//                  tid == 0 ? "main" : fmt::format("thread T{}", tid));
//
//  raw += header + '\n';
//
//  bandicoot::Stack out;
//  out.set_idx(static_cast<uint32_t>(mop_idx));
//  out.set_type(rw);
//  out.set_thread_info(header);
//  appendFrames(trace, out.mutable_trace());
//  raw += '\n';
//  return out;
//}

// -----------------------------------------------------------------------------
// FULL report builder
// -----------------------------------------------------------------------------
bandicoot::TsanReport BuildTsanReport(void* report) {
  bandicoot::TsanReport rep;
  std::string raw;

  // ── headline ---------------------------------------------------------------
  const char* desc = nullptr;
  int dup = 0, stack_cnt = 0, mop_cnt = 0, loc_cnt = 0, mutex_cnt = 0,
      thread_cnt = 0, uniq_cnt = 0;
  void* sleep_trace[TRACE_MAX] = {};
  __tsan_get_report_data(report, &desc, &dup, &stack_cnt, &mop_cnt, &loc_cnt,
                         &mutex_cnt, &thread_cnt, &uniq_cnt, sleep_trace,
                         TRACE_MAX);

  rep.set_description(desc ? desc : "");
  rep.set_duplicate_count(static_cast<uint32_t>(dup));
  for (int i = 0; i < TRACE_MAX && sleep_trace[i]; ++i) {
    appendFrames(sleep_trace, *rep.mutable_sleep_trace());
  }

  raw = "==================\n";
  raw += fmt::format("WARNING: ThreadSanitizer: {}\n", desc ? desc : "unknown");
  raw += "==================\n";

  // ── STACKS -----------------------------------------------------------------
  stack_cnt = std::min(stack_cnt, ARRAY_MAX);
  for (int i = 0; i < stack_cnt; ++i) {
    void* trace[TRACE_MAX] = {};
    __tsan_get_report_stack(report, i, trace, TRACE_MAX);
    auto* stk = rep.add_stacks();
    stk->set_idx(i);
    appendFrames(trace, *stk);
  }

  // ── MOPS (and remember first read/write) -----------------------------------
  int first_read = -1, first_write = -1;
  mop_cnt = std::min(mop_cnt, ARRAY_MAX);
  for (int i = 0; i < mop_cnt; ++i) {
    int tid = 0, sz = 0, wr = 0, at = 0;
    void* addr = nullptr;
    void* trace[TRACE_MAX] = {};
    __tsan_get_report_mop(report, i, &tid, &addr, &sz, &wr, &at, trace,
                          TRACE_MAX);

    bandicoot::Mop* mp = rep.add_mops();
    mp->set_idx(i);
    mp->set_tid(tid);
    mp->set_size(sz);
    mp->set_write(wr);
    mp->set_atomic(at);
    mp->set_addr(reinterpret_cast<uint64_t>(addr));
    appendFrames(trace, *mp->mutable_trace());

    if (wr && first_write == -1) first_write = i;
    if (!wr && first_read == -1) first_read = i;
  }

  // ── LOCATIONS --------------------------------------------------------------
  loc_cnt = std::min(loc_cnt, ARRAY_MAX);
  for (int i = 0; i < loc_cnt; ++i) {
    const char* type = nullptr;
    void *addr = nullptr, *start = nullptr;
    unsigned long sz = 0;
    int tid = 0, fd = 0, sup = 0;
    void* trace[TRACE_MAX] = {};
    __tsan_get_report_loc(report, i, &type, &addr, &start, &sz, &tid, &fd, &sup,
                          trace, TRACE_MAX);

    auto* lp = rep.add_locs();
    lp->set_idx(i);
    lp->set_type(type ? type : "");
    lp->set_addr(reinterpret_cast<uint64_t>(addr));
    lp->set_start(reinterpret_cast<uint64_t>(start));
    lp->set_size(sz);
    lp->set_tid(tid);
    lp->set_fd(fd);
    lp->set_suppressable(sup);
    appendFrames(trace, *lp->mutable_trace());
  }

  // ── MUTEXES ----------------------------------------------------------------
  mutex_cnt = std::min(mutex_cnt, ARRAY_MAX);
  for (int i = 0; i < mutex_cnt; ++i) {
    uint64_t mid = 0;
    void* addr = nullptr;
    int destroyed = 0;
    void* trace[TRACE_MAX] = {};
    __tsan_get_report_mutex(report, i, &mid, &addr, &destroyed, trace,
                            TRACE_MAX);

    auto* mp = rep.add_mutexes();
    mp->set_idx(i);
    mp->set_mutex_id(mid);
    mp->set_addr(reinterpret_cast<uint64_t>(addr));
    mp->set_destroyed(destroyed);
    appendFrames(trace, *mp->mutable_trace());
  }

  // ── THREADS ----------------------------------------------------------------
  thread_cnt = std::min(thread_cnt, ARRAY_MAX);
  for (int i = 0; i < thread_cnt; ++i) {
    int tid = 0, running = 0, ptid = 0;
    uint64_t osid = 0;
    const char* name = nullptr;
    void* trace[TRACE_MAX] = {};
    __tsan_get_report_thread(report, i, &tid, &osid, &running, &name, &ptid,
                             trace, TRACE_MAX);

    auto* tp = rep.add_threads();
    tp->set_idx(i);
    tp->set_tid(tid);
    tp->set_os_id(osid);
    tp->set_running(running);
    if (name) tp->set_name(name);
    tp->set_parent_tid(ptid);
    appendFrames(trace, *tp->mutable_trace());
  }

  // ── UNIQUE TIDs ------------------------------------------------------------
  uniq_cnt = std::min(uniq_cnt, ARRAY_MAX);
  for (int i = 0; i < uniq_cnt; ++i) {
    int utid = 0;
    __tsan_get_report_unique_tid(report, i, &utid);
    auto* ut = rep.add_unique_tids();
    ut->set_idx(i);
    ut->set_tid(utid);
  }

  // ── High-level read/write stacks for UX ------------------------------------
  raw += "==================\n";
  rep.set_raw_output(std::move(raw));
  return rep;
}

// RULES:
// Do not spawn fucking threads in here!!! TSAN will DEADLOCK.
extern "C" void __tsan_on_report(void* report) {
  const bandicoot::TsanReport pb_repr = BuildTsanReport(report);
  GetQueue().Dispatch([pb_repr]() {
    ::grpc::ClientContext context;
    ::bandicoot::Void response;
    ::grpc::Status status =
        GetClient().stub_->OnSanitizerReport(&context, pb_repr, &response);
    if (!status.ok()) {
      SPDLOG_WARN("Error sending gRPC: {} {} {}",
                  static_cast<int>(status.error_code()), status.error_details(),
                  status.error_message());
    }
  });
  std::string blob;
  pb_repr.SerializeToString(&blob);
  std::ofstream("tsan.bin", std::ios::binary).write(blob.data(), blob.size());
  SPDLOG_INFO("Sent: {}", pb_repr.DebugString());
}
