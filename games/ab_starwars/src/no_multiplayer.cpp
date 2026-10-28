// Multiplayer is not in this release of the game. Choosing Multiplayer on Play Mode Selection makes the menu's update
// (sub_822757E8, action 8) switch screens with sub_82256830("MultiplayerPlayModeSelect"); that screen's file,
// LEVELPAK:MultiplayerPlayModeSelect.lua, is not in levels.pak, so the game throws io::IOException ("Failed to open
// {0}"), which recompiled code cannot catch, and the process ends (found 28 Oct 2026 with guest_exceptions.cpp). The
// owner wants it gone: the switch to that screen is refused, so the button does nothing and the menu stays as it is.
// (The Kinect path to the same screen, sub_822CB4B0, is refused the same way.)

#include <atomic>
#include <cstdint>
#include <string>

#include <rex/hook.h>
#include <rex/logging.h>

namespace {

uint32_t U32(const uint8_t* base, uint32_t a) {
  return uint32_t(base[a]) << 24 | uint32_t(base[a + 1]) << 16 | uint32_t(base[a + 2]) << 8 | base[a + 3];
}

// The screen name object sub_82188250 builds: its text pointer first.
std::string SceneName(const uint8_t* base, uint32_t object) {
  const uint32_t text = U32(base, object);
  std::string s;
  if (text < 0x40000000 || text >= 0xA0000000) return s;
  for (uint32_t i = 0; i < 64 && base[text + i]; ++i) s += char(base[text + i]);
  return s;
}

}  // namespace

namespace absw {
void TitleShown();  // skip_intros.cpp
void MenuShown();   // skip_intros.cpp
}

REX_EXTERN(__imp__sub_82256830);
REX_HOOK_RAW(sub_82256830) {
  const std::string name = SceneName(base, ctx.r3.u32);
  REXLOG_INFO("ABSW: screen {}", name);
  if (name == "MainMenu") absw::TitleShown();     // skip_intros.cpp: the intros are over
  if (name == "EpisodeSelection") absw::MenuShown();  // skip_intros.cpp: past the menus
  if (name == "MultiplayerPlayModeSelect") {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) REXLOG_INFO("ABSW: Multiplayer is not in this release; the button does nothing");
    return;
  }
  __imp__sub_82256830(ctx, base);
}
