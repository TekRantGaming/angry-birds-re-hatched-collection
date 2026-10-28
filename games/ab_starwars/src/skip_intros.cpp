// Startup intros skipped (ab_skip_intros, on by default; owner, 28 Oct 2026): Play goes straight to the episode select,
// as the Trilogy tiles do.
//
// The start sequence screen (startSequenceScreen) lists its steps when it opens: sub_822E71D8 adds them by name with
// sub_821C0958(queue, name) in this order: AngryBirds (splash), Autosave (notice), Copyright (text and Dolby), then
// ActivisionVideo, RovioVideo, Lucas (LucasArts logo), ExientVideo and TitleVideo (data/videos/binkhd/*.bik). The
// logo and notice steps are left out. Two stay: the list must not be empty (the sequence then reads a null step), so
// the splash stays as the loading screen, and MainMenu reads what TitleVideo sets up (without it the switch to MainMenu
// reads a null pointer). The A taps below end both as soon as they can.
//
// MainMenu then waits for Start, and PlayModeMenu (Play Mode Selection) offers only Single Player, as Multiplayer is
// not in this release (no_multiplayer.cpp). Until the game switches to EpisodeSelection, the pad hook (input.cpp) taps
// A on user 0 every 0.4 s, which presses Start and then Single Player as a player would. Back from the episode select
// returns to Play Mode Selection as usual. The screens mark themselves from no_multiplayer.cpp. A minute and a half
// after the first tap it stops in any case.

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(ab_skip_intros, true, "Collection", "Skip the logos and intro movies at startup");

namespace absw {
namespace {
using Clock = std::chrono::steady_clock;
std::atomic<bool> g_menu{false};
thread_local bool g_listing_steps = false;  // inside sub_822E71D8

bool IsShownStep(const char* name) {
  static const char* const kSteps[] = {"Autosave", "Copyright", "ActivisionVideo", "RovioVideo", "Lucas", "ExientVideo"};
  for (const char* step : kSteps)
    if (std::strcmp(name, step) == 0) return true;
  return false;
}
}  // namespace

void TitleShown() {
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true) && REXCVAR_GET(ab_skip_intros)) REXLOG_INFO("ABSW: title shown, pressing Start");
}

void MenuShown() {
  if (!g_menu.exchange(true) && REXCVAR_GET(ab_skip_intros)) REXLOG_INFO("ABSW: episode select shown, intros skipped");
}

uint16_t SkipIntroButtons() {
  if (g_menu.load() || !REXCVAR_GET(ab_skip_intros)) return 0;
  static const Clock::time_point start = Clock::now();
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
  if (ms > 90000) return 0;
  return ms % 400 < 100 ? 0x1000 : 0;  // XINPUT_GAMEPAD_A
}

}  // namespace absw

REX_EXTERN(__imp__sub_822E71D8);
REX_HOOK_RAW(sub_822E71D8) {
  absw::g_listing_steps = true;
  __imp__sub_822E71D8(ctx, base);
  absw::g_listing_steps = false;
}

REX_EXTERN(__imp__sub_821C0958);
REX_HOOK_RAW(sub_821C0958) {
  if (absw::g_listing_steps && REXCVAR_GET(ab_skip_intros)) {
    const char* name = reinterpret_cast<const char*>(base + ctx.r4.u32);
    if (absw::IsShownStep(name)) {
      REXLOG_INFO("ABSW: start sequence step {} skipped", name);
      return;
    }
  }
  __imp__sub_821C0958(ctx, base);
}
