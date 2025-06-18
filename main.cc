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

#include <format>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
template <typename Range, typename DrawFn>
static void ShowArray(const char* name, const Range& range,
                      DrawFn&& draw_elem)  // <- accept any callable
{
  using std::begin;
  using std::end;
  const bool empty = (begin(range) == end(range));

  if (empty) {  // grey, non-openable header
    ImGui::BeginDisabled();
    const std::string lbl = std::format("{} <no information provided>", name);
    ImGui::TreeNodeEx(lbl.c_str(), ImGuiTreeNodeFlags_Leaf |
                                       ImGuiTreeNodeFlags_NoTreePushOnOpen);
    ImGui::EndDisabled();
    return;
  }

  if (ImGui::TreeNode(name)) {  // normal expandable branch
    if (draw_elem) {
      for (const auto& elem : range) {
        draw_elem(elem);
      }
    }
    ImGui::TreePop();
  }
}
// -----------------------------------------------------------------------------
// Pretty-print one stack’s frames as plain rows (no bullet)
// -----------------------------------------------------------------------------
static void DrawStackFrames(const instruments2::Stack& st) {
  ImGui::Indent();  // keep the block one level deeper
  for (int i = 0; i < st.frames_size(); ++i) {
    const auto& f = st.frames(i);

    //  #0  path/to/file.cc:123  in  myFunc()
    ImGui::Text("#%-2d  %s:%u  in  %s() [%s]", i, f.file_name().c_str(),
                f.line(), f.function().c_str(), f.repr().c_str());
  }
  ImGui::Unindent();
}
std::thread gServerThread;

std::mutex reports_mu_;
std::vector<instruments2::TsanReport> reports{};

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
  SPDLOG_INFO("HERE2");

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
                       SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
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
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
                                           // io.ConfigFlags |=
  //     ImGuiConfigFlags_NavEnableGamepad;             // Enable Gamepad
  //     Controls
  // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable
  io.ConfigFlags |= ImGuiWindowFlags_NoTitleBar;
  // Multi-Viewport io.ConfigViewportsNoAutoMerge = true;
  // io.ConfigViewportsNoTaskBarIcon = true;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform
  // windows can look identical to regular ones.
  ImGuiStyle& style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

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
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application, or clear/overwrite your copy of the
    // keyboard data. Generally you may always pass all inputs to dear imgui,
    // and hide them from your application based on those two flags.
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

    ImGui::Begin("Instruments2");
    for (size_t i = 0; i < reports.size(); ++i) {
      auto& r = reports[i];
      ImGui::PushID(static_cast<int>(i));  // ensure stable IDs

      std::string hdr = std::format("[{}]  {}", i, r.description());
      if (ImGui::CollapsingHeader(hdr.c_str(),
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
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
          const std::string tid =
              m.tid() == 0 ? "0 (main thread)" : std::format("{}", m.tid());

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
              // title:   <type>  @0xSTART-0xEND  size:N  tid:M  [suppressible]
              std::string label = std::format(
                  "{}  @0x{:X}-0x{:X}  size:{}B  tid:{}{}", loc.type(),
                  loc.start(), loc.start() + loc.size(), loc.size(), loc.tid(),
                  loc.suppressable() ? "  [suppressible]" : "");

              if (ImGui::TreeNode(label.c_str())) {
                if (loc.fd() >= 0) ImGui::Text("fd: %d", loc.fd());

                DrawStackFrames(loc.trace());  // <-- helper from earlier
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
        ShowArray("Threads", r.threads(),
                  [](const instruments2::ThreadInfo& th) {
                    // title:   tid:N  os:N  running/blocked  "name"
                    const std::string tid = th.tid() == 0
                                                ? "0 (main thread)"
                                                : std::format("{}", th.tid());
                    const std::string name =
                        th.name().empty() ? "<unnamed thread>" : th.name();
                    const std::string label =
                        std::format("tid:{} os:{} state={} {}", tid, th.os_id(),
                                    th.running() ? "running" : "blocked", name);

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
    ImGui::End();

    // 1. Show the big demo window (Most of the sample code is in
    // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
    // ImGui!).
    ImGui::ShowDemoWindow(&show_demo_window);

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we
    // save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call SDL_GL_MakeCurrent(window,
    //  gl_context) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
      SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

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
