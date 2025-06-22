// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows,
// inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/
// folder).
// - Introduction, links and more at the top of imgui.cpp

#include <SDL.h>
#include <SDL_opengl.h>
#include <pbtypes/instruments2.pb.h>
#include <stdio.h>
#include <tsb/ipc.h>
#include <tsb/log_reporter.h>

#include <filesystem>
#include <format>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#if defined(_WIN32)
const char* kSystemFont = "C:\\Windows\\Fonts\\segoeui.ttf";
#elif defined(__APPLE__)
const char* kSystemFont = "/System/Library/Fonts/SFNSMono.ttf";
#else
const char* kSystemFont = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif

template <typename Range, typename DrawFn>
static void ShowArray(const char* name, const Range& range,
                      DrawFn&& draw_elem) {
  using std::begin;
  using std::end;
  const bool empty = (begin(range) == end(range));

  if (empty) {
    return;
    // TODO(bojanin): Receive UX feedback on this....?
    // I like the verbosity, but others perhaps wont?

    // ImGui::BeginDisabled();
    // const std::string lbl = std::format("{} <no information provided>",
    // name); ImGui::TreeNodeEx(lbl.c_str(), ImGuiTreeNodeFlags_Leaf |
    //                                    ImGuiTreeNodeFlags_NoTreePushOnOpen);
    // ImGui::EndDisabled();
    // return;
  }

  if (ImGui::TreeNode(name)) {  // normal expandable branch
    for (const auto& elem : range) {
      draw_elem(elem);
    }
    ImGui::TreePop();
  }
}
static std::string MakeTid(const uint32_t tid) {
  if (tid == 0) {
    return "0 (main thread)";
  }
  return std::format("{}", tid);
}
// -----------------------------------------------------------------------------
// Pretty-print one stack’s frames as plain rows (no bullet)
// -----------------------------------------------------------------------------
static void DrawStackFrames(const instruments2::Stack& st) {
  ImGui::Indent();  // keep the block one level deeper
  for (int i = 0; i < st.frames_size(); ++i) {
    const auto& f = st.frames(i);

    //  #0  path/to/file.cc:123  in  myFunc() (libmylib+offset)
    ImGui::Text("#%-2d  %s:%u in %s() %s", i, f.file_name().c_str(), f.line(),
                f.function().c_str(), f.repr().c_str());
  }
  ImGui::Unindent();
}
std::thread gServerThread;

std::mutex reports_mu_;
std::vector<instruments2::TsanReport> reports{};
void SetupColors() {
  ImGui::GetStyle().FrameRounding = 4.0f;
  ImGui::GetStyle().GrabRounding = 4.0f;

  ImVec4* colors = ImGui::GetStyle().Colors;
  colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
  colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
  colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
  colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

// Main code
int main(int, char**) {
  reports.reserve(100);
  tsb::LogReporter::Create();
  tsb::IPCServer::Create();
  gServerThread = std::thread([]() {
    tsb::IPCServer::Shared()->AddTsanHandler(
        [](const instruments2::TsanReport& report) {
          std::scoped_lock<std::mutex> sl{reports_mu_};
          reports.emplace_back(report);
        });

    tsb::IPCServer::Shared()->RunForever();
  });
  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
      0) {
    printf("Error: %s\n", SDL_GetError());
    printf("123");
    return -1;
  }

  // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char* glsl_version = "#version 100";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
  // GL 3.2 Core + GLSL 150
  const char* glsl_version = "#version 150";
  SDL_GL_SetAttribute(
      SDL_GL_CONTEXT_FLAGS,
      SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);  // Always required on Mac
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
  // GL 3.0 + GLSL 130
  const char* glsl_version = "#version 130";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

  // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window* window =
      SDL_CreateWindow("Instruments2", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 1920, 1080, window_flags);
  if (window == nullptr) {
    printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    return -1;
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1);  // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->Clear();
  const bool exists = std::filesystem::exists(kSystemFont);
  if (!exists) {
    SPDLOG_WARN("{} doesnt exist...", kSystemFont);
  }

  ImFont* system_font = io.Fonts->AddFontFromFileTTF(
      kSystemFont, 18, NULL, io.Fonts->GetGlyphRangesDefault());
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

  // DEBUG TOOLS:
  // https://github.com/ocornut/imgui/wiki/Debug-Tools
  io.ConfigDebugHighlightIdConflicts = true;

  // Setup Dear ImGui style
  SetupColors();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Our state
  bool show_demo_window = true;
  // bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  SPDLOG_INFO("starting main loop!");

  // Main loop
  bool done = false;
  std::vector<std::string> headers = {"Stack Frame", "File Info", "Extra"};
  while (!done) {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data
    // to your main application, or clear/overwrite your copy of the mouse
    // data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application, or clear/overwrite your copy of the
    // keyboard data. Generally you may always pass all inputs to dear
    // imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) done = true;
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window))
        done = true;
    }
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
      SDL_Delay(10);
      continue;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    ImGui::Begin("Main Window", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PushFont(system_font);

    ImVec2 display_size = ImGui::GetContentRegionAvail();

    // Left child for current view (reports)
    ImGui::BeginChild("Left", ImVec2(display_size.x * 0.5f, 0), true);
    for (size_t i = 0; i < reports.size(); ++i) {
      auto& r = reports[i];
      ImGui::PushID(static_cast<int>(i));  // ensure stable IDs

      std::string hdr = std::format("[{}]  {}", i, r.description());
      if (ImGui::CollapsingHeader(hdr.c_str())) {
        // ── Stacks ────────────────────────────────
        ShowArray("Stacks", r.stacks(), [](const instruments2::Stack& s) {
          // Each Stack is itself a mini-tree
          std::string label = std::format("Stack #{}", s.idx());
          if (ImGui::TreeNode(label.c_str())) {
            DrawStackFrames(s);
            ImGui::TreePop();
          }
        });

        // ── Memory ops ───────────────────────────
        ShowArray("Memory Accesses", r.mops(), [](const instruments2::Mop& m) {
          // WRITE / READ + optional “atomic” tag
          const char* op = m.write() ? "Write" : "Read";
          const char* atom = m.atomic() ? " (atomic)" : "";
          const std::string tid = MakeTid(m.tid());

          // 0xADDR + size + tid
          const std::string lbl = std::format(
              "{}{} addr: 0x{:X}  size: {}B  tid:{}",  // <-- everything but
                                                       // idx & trace
              op, atom, m.addr(), m.size(), tid);
          if (ImGui::TreeNode(lbl.c_str())) {
            DrawStackFrames(m.trace());
            ImGui::TreePop();
          }
        });

        // Locs, Mutexes, Threads, … (same pattern) …
        ShowArray(
            "Memory Locations", r.locs(), [](const instruments2::Loc& loc) {
              // title:   location=<type>  @0xSTART-0xEND  size:N  tid:M
              // [suppressible] <no stack trace if none exists>
              const std::string label =
                  std::format("location={}  0x{:X}-0x{:X}  size:{}B  tid:{}{}",
                              loc.type(), loc.start(), loc.start() + loc.size(),
                              loc.size(), MakeTid(loc.tid()),
                              loc.suppressable() ? "  [suppressible]" : "");

              if (ImGui::TreeNode(label.c_str())) {
                // TODO(bojanin): should this be >2, should we show all?
                if (loc.fd() > 0) {
                  ImGui::Text("fd: %d", loc.fd());
                }

                if (loc.trace().frames().empty()) {
                  ImGui::Text("<No stack trace available>");
                } else {
                  DrawStackFrames(loc.trace());
                }
                ImGui::TreePop();
              }
            });
        ShowArray("Mutexes", r.mutexes(),
                  [](const instruments2::MutexInfo& mut) {
                    // title:   mutex:0xID  addr:0xADDR  [destroyed]
                    std::string label = std::format(
                        "mutex:0x{:X}  addr:0x{:X}{}", mut.mutex_id(),
                        mut.addr(), mut.destroyed() ? "  [destroyed]" : "");

                    if (ImGui::TreeNode(label.c_str())) {
                      DrawStackFrames(mut.trace());
                      ImGui::TreePop();
                    }
                  });
        ShowArray(
            "Threads", r.threads(), [&r](const instruments2::ThreadInfo& th) {
              // title:   tid:N  os:N  running/blocked  "name" <num threads
              // leaked | null>
              const std::string tid = th.tid() == 0
                                          ? "0 (main thread)"
                                          : std::format("{}", th.tid());
              const std::string duplicates =
                  std::format("{} duplicate threads", r.duplicate_count());
              const std::string name =
                  th.name().empty() ? "<unnamed thread>" : th.name();
              const std::string label = std::format(
                  "tid:{} os:{} state={} {} ({})", tid, th.os_id(),
                  th.running() ? "running" : "blocked", name, duplicates);

              if (ImGui::TreeNode(label.c_str())) {
                if (th.trace().frames().empty()) {
                  ImGui::Text("parent tid: %u <no stack trace>",
                              th.parent_tid());
                } else {
                  DrawStackFrames(th.trace());
                }
                ImGui::TreePop();
              }
            });
      }
      ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("Right", ImVec2(0, 0), true);
    ImGui::BeginChild("Terminal", ImVec2(0, display_size.y * 0.5f), true);
    if (1 == 2) {
      ImGui::TextUnformatted("AHHHH THIS IS TERMINAL OUTPUT");
    } else {
      ImGui::Text("<No sanitizer output>");
    }
    ImGui::EndChild();

    // Bottom right for code search
    ImGui::BeginChild("CodeSearch", ImVec2(0, 0),
                      true);  // 0,0 takes remaining space
    ImGui::Text("<Symbol location not found>");
    ImGui::EndChild();

    // 1. Show the big demo window (Most of the sample code is in
    // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
    // ImGui!).
    ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::EndChild();
    ImGui::PopFont();
    ImGui::End();
    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
