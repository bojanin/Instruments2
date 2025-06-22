#include <sanitizer/common_interface_defs.h>
#include <sanitizer/tsan_interface.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <tsb/constants.h>
#include <tsb/log_reporter.h>

#ifdef __APPLE__
#include <dlfcn.h>
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

#include <tsb/dispatch_queue.h>
#include <tsb/ipc.h>

static char gSymbolicationScratchPad[8196];

// NOTE(bojanin): this leaks memory but we can't use unique_ptr because it will
// be destructed before the sanitizer compiler runtime finishes finding issues.
// TODO(bojanin): potentially parameterize this? i suspect memsan will complain
// about the raw ptr?
std::atomic_bool deinit = false;
static tsb::DispatchQueue& GetQueue() {
  static tsb::DispatchQueue* q = new tsb::DispatchQueue();
  return *q;
}

static tsb::IPCClient& GetClient() {
  static tsb::IPCClient* c = new tsb::IPCClient();
  return *c;
}

__attribute__((constructor)) void Init() {
  tsb::LogReporter::Create();
  // Spin up threads in contructor because tsan will deadlock if you do it
  // before.
  (void)GetClient();
  GetQueue().Dispatch([]() {});
  SPDLOG_INFO("Instruments2: Shlib Init");
}

__attribute__((destructor)) void Deinit() {
  deinit = true;
  SPDLOG_INFO("Instruments2: Shlib Deinit");
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

// -----------------------------------------------------------------------------
//  PC → StackFrame
// -----------------------------------------------------------------------------
static instruments2::StackFrame MakeFrame(void* pc) {
  // TODO(bojanin): Consider extracting out each line into the buffer and
  // populating that, then prettifying it client side?
  static char buf[2048] = {};
  std::memset(buf, 0, sizeof(buf));
  // "%s:%l:%f"  →  "file.cc:42:func"
  __sanitizer_symbolize_pc(pc, "%s:%l:%f", buf, sizeof(buf));

  instruments2::StackFrame f;

  const char* c1 = strchr(buf, ':');
  const char* c2 = c1 ? strchr(c1 + 1, ':') : nullptr;
  if (c1 && c2) {
    f.set_file_name({buf, static_cast<size_t>(c1 - buf)});
    f.set_line(static_cast<uint32_t>(std::atoi(c1 + 1)));
    f.set_function(c2 + 1);
  }

  std::memset(buf, 0, sizeof(buf));
  __sanitizer_symbolize_pc(pc, "%M", buf, sizeof(buf));
  f.set_repr(buf);
  return f;
}

// Append user frames from |trace| into proto repeated<StackFrame>.
static void AppendFrames(void** trace, instruments2::Stack& dst) {
  for (int i = 0; trace[i] && i < tsb::TRACE_MAX; ++i) {
    *dst.add_frames() = MakeFrame(trace[i]);
  }
}

// -----------------------------------------------------------------------------
// Build a Stack from one MOP index & emit human text to |raw|
// -----------------------------------------------------------------------------
// static instruments2::Stack buildStack(void* report, unsigned long mop_idx,
//                                   std::string& raw) {
//  int tid = 0, sz = 0, wr = 0, at = 0;
//  void* addr = nullptr;
//  void* trace[captain_hook::TRACE_MAX] = {};
//  __tsan_get_report_mop(report, mop_idx, &tid, &addr, &sz, &wr, &at, trace,
//                        captain_hook::TRACE_MAX);
//
//  const char* rw = wr ? "Write" : "Read";
//  std::string header =
//      fmt::format("  {} of size {} at {:#018x} by {} thread:", rw, sz,
//                  reinterpret_cast<std::uintptr_t>(addr),
//                  tid == 0 ? "main" : fmt::format("thread T{}", tid));
//
//  raw += header + '\n';
//
//  instruments2::Stack out;
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
instruments2::TsanReport BuildTsanReport(void* report) {
  instruments2::TsanReport rep;
  std::string raw;

  // ── headline ---------------------------------------------------------------
  const char* desc = nullptr;
  int dup = 0, stack_cnt = 0, mop_cnt = 0, loc_cnt = 0, mutex_cnt = 0,
      thread_cnt = 0, uniq_cnt = 0;
  void* sleep_trace[tsb::TRACE_MAX] = {};
  __tsan_get_report_data(report, &desc, &dup, &stack_cnt, &mop_cnt, &loc_cnt,
                         &mutex_cnt, &thread_cnt, &uniq_cnt, sleep_trace,
                         tsb::TRACE_MAX);

  rep.set_description(desc ? desc : "");
  rep.set_duplicate_count(static_cast<uint32_t>(dup));
  for (int i = 0; i < tsb::TRACE_MAX && sleep_trace[i]; ++i) {
    AppendFrames(sleep_trace, *rep.mutable_sleep_trace());
  }

  raw = "==================\n";
  raw += fmt::format("WARNING: ThreadSanitizer: {}\n", desc ? desc : "unknown");
  raw += "==================\n";

  // ── STACKS -----------------------------------------------------------------
  stack_cnt = std::min(stack_cnt, tsb::ARRAY_MAX);
  for (int i = 0; i < stack_cnt; ++i) {
    void* trace[tsb::TRACE_MAX] = {};
    __tsan_get_report_stack(report, i, trace, tsb::TRACE_MAX);
    auto* stk = rep.add_stacks();
    stk->set_idx(i);
    AppendFrames(trace, *stk);
  }

  // ── MOPS (and remember first read/write) -----------------------------------
  int first_read = -1, first_write = -1;
  mop_cnt = std::min(mop_cnt, tsb::ARRAY_MAX);
  for (int i = 0; i < mop_cnt; ++i) {
    int tid = 0, sz = 0, wr = 0, at = 0;
    void* addr = nullptr;
    void* trace[tsb::TRACE_MAX] = {};
    __tsan_get_report_mop(report, i, &tid, &addr, &sz, &wr, &at, trace,
                          tsb::TRACE_MAX);

    instruments2::Mop* mp = rep.add_mops();
    mp->set_idx(i);
    mp->set_tid(tid);
    mp->set_size(sz);
    mp->set_write(wr);
    mp->set_atomic(at);
    mp->set_addr(reinterpret_cast<uint64_t>(addr));
    AppendFrames(trace, *mp->mutable_trace());

    if (wr && first_write == -1) first_write = i;
    if (!wr && first_read == -1) first_read = i;
  }

  // ── LOCATIONS --------------------------------------------------------------
  loc_cnt = std::min(loc_cnt, tsb::ARRAY_MAX);
  for (int i = 0; i < loc_cnt; ++i) {
    const char* type = nullptr;
    void *addr = nullptr, *start = nullptr;
    unsigned long sz = 0;
    int tid = 0, fd = 0, sup = 0;
    void* trace[tsb::TRACE_MAX] = {};
    __tsan_get_report_loc(report, i, &type, &addr, &start, &sz, &tid, &fd, &sup,
                          trace, tsb::TRACE_MAX);

    auto* lp = rep.add_locs();
    lp->set_idx(i);
    lp->set_type(type ? type : "");
    lp->set_addr(reinterpret_cast<uint64_t>(addr));
    lp->set_start(reinterpret_cast<uint64_t>(start));
    lp->set_size(sz);
    lp->set_tid(tid);
    lp->set_fd(fd);
    lp->set_suppressable(sup);
    AppendFrames(trace, *lp->mutable_trace());
  }

  // ── MUTEXES ----------------------------------------------------------------
  mutex_cnt = std::min(mutex_cnt, tsb::ARRAY_MAX);
  for (int i = 0; i < mutex_cnt; ++i) {
    uint64_t mid = 0;
    void* addr = nullptr;
    int destroyed = 0;
    void* trace[tsb::TRACE_MAX] = {};
    __tsan_get_report_mutex(report, i, &mid, &addr, &destroyed, trace,
                            tsb::TRACE_MAX);

    auto* mp = rep.add_mutexes();
    mp->set_idx(i);
    mp->set_mutex_id(mid);
    mp->set_addr(reinterpret_cast<uint64_t>(addr));
    mp->set_destroyed(destroyed);
    AppendFrames(trace, *mp->mutable_trace());
  }

  // ── THREADS ----------------------------------------------------------------
  thread_cnt = std::min(thread_cnt, tsb::ARRAY_MAX);
  for (int i = 0; i < thread_cnt; ++i) {
    int tid = 0, running = 0, ptid = 0;
    uint64_t osid = 0;
    const char* name = nullptr;
    void* trace[tsb::TRACE_MAX] = {};
    __tsan_get_report_thread(report, i, &tid, &osid, &running, &name, &ptid,
                             trace, tsb::TRACE_MAX);

    auto* tp = rep.add_threads();
    tp->set_idx(i);
    tp->set_tid(tid);
    tp->set_os_id(osid);
    tp->set_running(running);
    if (name) {
      tp->set_name(name);
    }
    tp->set_parent_tid(ptid);
    AppendFrames(trace, *tp->mutable_trace());
  }

  // ── UNIQUE TIDs ------------------------------------------------------------
  uniq_cnt = std::min(uniq_cnt, tsb::ARRAY_MAX);
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
extern "C" int __tsan_on_finalize(int failed) {
  std::println("Tsan finalized: {}", failed);
  return failed;
}

// NOTE(bojanin): Do not spawn fucking threads in here!!! TSAN will DEADLOCK.
extern "C" void __tsan_on_report(void* report) {
  const instruments2::TsanReport pb_repr = BuildTsanReport(report);

  const auto rpc_msg = [pb_repr]() {
    ::grpc::ClientContext context;
    ::instruments2::Void response;
    ::grpc::Status status =
        GetClient().stub_->OnSanitizerReport(&context, pb_repr, &response);
    if (!status.ok()) {
      std::println("Error sending gRPC: {} {} {}",
                   static_cast<int>(status.error_code()),
                   status.error_details(), status.error_message());
    }
  };

  // NOTE(bojanin): the c++ runtime will deinit our shlib but the compiler
  // runtime can still call this function after atexit() has been called.
  if (deinit) {
    rpc_msg();
  } else {
    GetQueue().Dispatch(rpc_msg);
  }
}
