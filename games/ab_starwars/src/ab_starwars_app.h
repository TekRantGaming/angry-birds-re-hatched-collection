// ab_starwars - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <functional>

#include <rex/audio/sdl/sdl_audio_system.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/rex_app.h>
#include <rex/runtime.h>
#include <rex/ui/overlay/achievement_notification.h>
#include <rex/ui/overlay/debug_overlay.h>
#include <rex/ui/window.h>

namespace absw {
void InstallFpeGuard();  // fpe_guard.cpp
void ApplyPortDefaults();  // port_defaults.cpp
void ApplyRenderQuality(rex::ui::Window* window);  // render_quality.cpp
void InstallDialogPad(rex::ui::ImGuiDrawer* drawer, std::function<rex::Runtime*()> runtime);  // dialog_pad.cpp
bool OwnsForegroundWindow();  // platform.cpp
rex::ui::FrameStats GuestFrameStats();  // frame_stats.cpp
std::unique_ptr<rex::ui::AchievementNotificationDialog> CreateAchievementToast(
    rex::ui::ImGuiDrawer* drawer, rex::ui::ImmediateDrawer* immediate, rex::Runtime* runtime);  // achievement_toast.cpp
void ExportAchievements(rex::Runtime* runtime, const std::filesystem::path& game_data_root);  // achievements_export.cpp
void StartDevTools(rex::Runtime* runtime, rex::ui::WindowedAppContext& app_context, uint32_t image_base,
                   uint32_t image_size);  // dev_tools.cpp
}  // namespace absw

class AbStarwarsApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<AbStarwarsApp>(new AbStarwarsApp(ctx, "ab_starwars",
        PPCImageConfig));
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (!config.graphics && config.gpu_plugin.empty()) config.gpu_plugin = "xenos";
    if (!config.audio_factory) config.audio_factory = REX_AUDIO_BACKEND(rex::audio::sdl::SDLAudioSystem);
  }

  // After the GPU plugin has registered its settings, before the GPU starts (port_defaults.cpp).
  std::optional<rex::PathConfig> OnFinalizePaths(const rex::PathConfig& defaults,
                                                 std::function<void(rex::PathConfig)> resume) override {
    (void)resume;
    absw::ApplyPortDefaults();
    absw::ApplyRenderQuality(window());  // the upscaler mode's draw scale, before the GPU starts
    return defaults;
  }

  // ReXGlue reads the pads only while its window tracks itself as focused, and that flag can stay false for a window
  // that is in front and receiving input (seen when the game is started by another program): every pad then reads as
  // idle until the window is clicked. Read the pads whenever the window has focus by either measure: its own flag, or
  // the OS foreground window being ours (still not while another program is in front). From the Doritos Crash Course
  // port.
  void InstallInputFocusRule() {
    auto* input = static_cast<rex::input::InputSystem*>(runtime()->input_system());
    REXLOG_INFO("ABSW: input focus rule {}", input ? "installed" : "not installed (no input system)");
    if (!input) return;
    input->SetActiveCallback([window = window()] {
      const bool active = (window && window->HasFocus()) || absw::OwnsForegroundWindow();
      static std::atomic<int> logged{-1};
      if (logged.exchange(active) != int(active))
        REXLOG_INFO("ABSW: controller input {}", active ? "on (game window in front)" : "off (another window in front)");
      return active;
    });
  }

  // The controller answers the system message boxes the runtime draws (dialog_pad.cpp).
  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override { absw::InstallDialogPad(drawer, [this] { return runtime(); }); }

  void OnPostSetup() override {
    absw::InstallFpeGuard();  // after the runtime's own signal handlers
    InstallInputFocusRule();
    SetGuestFrameStats(absw::GuestFrameStats);  // the F3 overlay's frame rate
    absw::ExportAchievements(runtime(), game_data_root());  // for the collection launcher's Achievements page
    absw::StartDevTools(runtime(), app_context(), PPCImageConfig.image_base, PPCImageConfig.image_size);
  }

  // Achievement notifications in the TRG ports' style (achievement_toast.cpp).
  std::unique_ptr<rex::ui::AchievementNotificationDialog> CreateAchievementNotificationDialog() override {
    return absw::CreateAchievementToast(imgui_drawer(), immediate_drawer(), runtime());
  }
};
