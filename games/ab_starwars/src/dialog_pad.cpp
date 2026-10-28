// Controller for the system message boxes.
//
// The game shows some notices through XShowMessageBoxUI (sub_8258DBA8), for example "Not Connected: In order to view
// the leaderboards, you must be connected to Xbox LIVE" with one OK button right after Start on the title screen, and
// waits until it closes. The runtime draws that box as an ImGui popup that only a mouse click closes, and the game's
// own pad reading stops while it waits, so a player with only a controller was stuck. On the console the controller
// answers these boxes. While any popup is open, this feeds the controller to ImGui's gamepad navigation: A presses the
// focused button (the box focuses its default button), the D pad and left stick move between buttons, B goes back.
// Nothing changes while no popup is open.

#include <rex/input/input.h>
#include <rex/logging.h>
#include <rex/input/input_system.h>
#include <rex/runtime.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>

#include <functional>

#include <imgui.h>
#include <imgui_internal.h>

namespace absw {
uint16_t ScriptedButtonsNow();  // input.cpp (developer scripted pad)

namespace {

class DialogPad final : public rex::ui::ImGuiDialog {
 public:
  DialogPad(rex::ui::ImGuiDrawer* drawer, std::function<rex::Runtime*()> runtime)
      : ImGuiDialog(drawer), runtime_(std::move(runtime)) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    const bool popup = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    if (!popup) {
      if (feeding_) Release(io);
      return;
    }
    rex::Runtime* runtime = runtime_();
    auto* input = runtime ? static_cast<rex::input::InputSystem*>(runtime->input_system()) : nullptr;
    if (!input) return;
    uint16_t buttons = ScriptedButtonsNow();
    int lx = 0, ly = 0;
    for (uint32_t user = 0; user < 4; ++user) {  // any connected pad
      rex::input::X_INPUT_STATE state{};
      if (input->GetState(user, &state) != 0) continue;
      buttons |= uint16_t(state.gamepad.buttons);
      if (!lx) lx = int16_t(state.gamepad.thumb_lx);
      if (!ly) ly = int16_t(state.gamepad.thumb_ly);
    }
    if (!feeding_ || buttons != last_buttons_) {
      ImGuiContext& g = *ImGui::GetCurrentContext();
      REXLOG_INFO("ABSW: system box open, buttons {:04X}, imgui A down {}, nav window '{}', nav id {:08X}, active {:08X}, "
                  "config {:X}, backend {:X}, focused {}",
                  buttons, ImGui::IsKeyDown(ImGuiKey_GamepadFaceDown), g.NavWindow ? g.NavWindow->Name : "-", g.NavId,
                  g.ActiveId, int(io.ConfigFlags), int(io.BackendFlags), io.AppFocusLost ? "lost" : "yes");
    }
    last_buttons_ = buttons;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    // ImGui activates the focused button only while its navigation cursor shows, which otherwise needs a D pad move
    // first; showing it also marks the button A will press.
    ImGui::SetNavCursorVisible(true);
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    feeding_ = true;
    constexpr int kDead = 12000;
    io.AddKeyEvent(ImGuiKey_GamepadFaceDown, (buttons & 0x1000) != 0);   // A
    io.AddKeyEvent(ImGuiKey_GamepadFaceRight, (buttons & 0x2000) != 0);  // B
    io.AddKeyEvent(ImGuiKey_GamepadDpadUp, (buttons & 0x0001) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadDpadDown, (buttons & 0x0002) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadDpadLeft, (buttons & 0x0004) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadDpadRight, (buttons & 0x0008) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadLStickLeft, lx < -kDead);
    io.AddKeyEvent(ImGuiKey_GamepadLStickRight, lx > kDead);
    io.AddKeyEvent(ImGuiKey_GamepadLStickUp, ly > kDead);
    io.AddKeyEvent(ImGuiKey_GamepadLStickDown, ly < -kDead);
  }

 private:
  void Release(ImGuiIO& io) {
    for (ImGuiKey key : {ImGuiKey_GamepadFaceDown, ImGuiKey_GamepadFaceRight, ImGuiKey_GamepadDpadUp,
                         ImGuiKey_GamepadDpadDown, ImGuiKey_GamepadDpadLeft, ImGuiKey_GamepadDpadRight,
                         ImGuiKey_GamepadLStickLeft, ImGuiKey_GamepadLStickRight, ImGuiKey_GamepadLStickUp,
                         ImGuiKey_GamepadLStickDown})
      io.AddKeyEvent(key, false);
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    feeding_ = false;
  }

  std::function<rex::Runtime*()> runtime_;  // the app's runtime (made after the dialogs)
  bool feeding_ = false;
  uint16_t last_buttons_ = 0;
};

}  // namespace

void InstallDialogPad(rex::ui::ImGuiDrawer* drawer, std::function<rex::Runtime*()> runtime) {
  new DialogPad(drawer, std::move(runtime));  // lives for the rest of the session
}

}  // namespace absw
