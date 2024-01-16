#include "SDL.h"
#include "SDL_opengl.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"

// Audio library.
#include "libAudio/AudioContext.h"
#include "libAudio/AudioEngine.h"
#include "libAudio/SoundLoader.h"
#include "libAudio/SoundInstance.h"
#include "libAudio/Sound.h"

#define STB_IMAGE_IMPLEMENTATION
#include "STB/stb_image.h"

#include <cstdio>
#include <functional>
#include <memory>
#include <random>
#include <sstream>
#include <string>

class ToolMouseInfo {
public:
  uint32_t LastState = 0;
  uint32_t State = 0;
  int X = 0, Y = 0;
  int DX = 0, DY = 0;

  void Initialize() { LastState = State = SDL_GetMouseState(&X, &Y); }

  void Update() {
    LastState = State;
    State = SDL_GetMouseState(&X, &Y);
    SDL_GetRelativeMouseState(&DX, &DY);
  }

  [[nodiscard]] bool LeftUp() const {
    return (SDL_BUTTON_LMASK & State) && !(SDL_BUTTON_LMASK & LastState);
  }

  [[nodiscard]] bool MiddleUp() const {
    return (SDL_BUTTON_MMASK & State) && !(SDL_BUTTON_MMASK & LastState);
  }

  [[nodiscard]] bool RightUp() const {
    return (SDL_BUTTON_RMASK & State) && !(SDL_BUTTON_RMASK & LastState);
  }

  [[nodiscard]] bool LeftDown() const { return SDL_BUTTON_LMASK & State; }

  [[nodiscard]] bool MiddleDown() const { return SDL_BUTTON_MMASK & State; }

  [[nodiscard]] bool RightDown() const { return SDL_BUTTON_RMASK & State; }
};

class ToolView {
public:
  ToolMouseInfo MouseInfo;

  ImVec2 WindowSize = ImVec2(1280, 720);

  ImVec2 WindowScroll = ImVec2(0, 0);
  ImVec2 CameraPosition = ImVec2(0, 0);

  const char *StatusText = nullptr;
  std::function<void(ImVec2)> LeftClickCallback;

  void AssignLeftClickMode(const char *InText,
    const std::function<void(ImVec2)> &InCallback) {
    StatusText = InText;
    LeftClickCallback = [InCallback, this](const ImVec2 pos) {
      InCallback(pos);
      StatusText = nullptr;
      LeftClickCallback = nullptr;
    };
  }

  void HandleMouse() const {
    if (MouseInfo.LeftUp() && LeftClickCallback)
      LeftClickCallback(ImVec2(WindowScroll.x + static_cast<float>(MouseInfo.X),
        WindowScroll.y + static_cast<float>(MouseInfo.Y)));
  }

  void Update() {
    MouseInfo.Update();
    HandleMouse();
  }

  static void InitializeGraphics() {}

  void Initialize() {
    MouseInfo.Initialize();
    InitializeGraphics();
  }

  [[nodiscard]] ImVec2 ScrolledPosition(const float x, const float y) const {
    return { x - WindowScroll.x, y - WindowScroll.y };
  }

  [[nodiscard]] ImVec2 ScrolledPosition(const ImVec2 &v) const {
    return ScrolledPosition(v.x, v.y);
  }

  void Draw(std::stringstream &statusStream) const {
    if (StatusText != nullptr)
      statusStream = std::stringstream();

    statusStream << "(" << MouseInfo.X << "," << MouseInfo.Y << ")";
    if (StatusText != nullptr)
      statusStream << ", " << StatusText;

    // Mouse coordinates
    ImGui::SetNextWindowPos(ImVec2(5, WindowSize.y - 30));
    ImGui::Begin("Status", nullptr,
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_AlwaysAutoResize);

    const std::string statusStr(statusStream.str());
    ImGui::Text("%s", statusStr.c_str());
    ImGui::End();
  }
};

int main(int argc, char *argv[]) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
    0) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }

  ToolView toolView;

  // GL 3.0 + GLSL 130
  const auto glsl_version = "#version 130";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  constexpr auto windowFlags = static_cast<SDL_WindowFlags>(
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  SDL_Window *window = SDL_CreateWindow("OpenGL", SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED, static_cast<int>(toolView.WindowSize.x),
    static_cast<int>(toolView.WindowSize.y), windowFlags);

  const SDL_GLContext glContext = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, glContext);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  // Initialize graphics counterparts
  toolView.Initialize();

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window, glContext);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // TODO: User initialization

  // Main loop
  uint64_t CurrentTicks = SDL_GetTicks64();
  bool done = false;
  while (!done) {
    const uint64_t TicksNow = SDL_GetTicks64();
    const uint64_t TicksDelta = TicksNow - CurrentTicks;
    const float DeltaTime = static_cast<float>(TicksDelta) / 1000.0f;
    CurrentTicks = TicksNow;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        done = true;

      if (event.type == SDL_WINDOWEVENT &&
        event.window.windowID == SDL_GetWindowID(window)) {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE)
          done = true;

        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          toolView.WindowSize.x = static_cast<float>(event.window.data1);
          toolView.WindowSize.y = static_cast<float>(event.window.data2);
          toolView.InitializeGraphics();
        }
      }
    }

    toolView.Update();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    {
      ImGui::SetNextWindowPos(ImVec2(0, 0));
      ImGui::SetNextWindowSize(toolView.WindowSize);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
      ImGui::Begin("Main", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration |
          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
          ImGuiWindowFlags_NoBringToFrontOnFocus);

      std::stringstream statusStream;
      toolView.Draw(statusStream);

      ImGui::End();
      ImGui::PopStyleVar(2);
    }

    // Rendering
    ImGui::Render();
    glViewport(0, 0, static_cast<int>(io.DisplaySize.x),
      static_cast<int>(io.DisplaySize.y));

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
