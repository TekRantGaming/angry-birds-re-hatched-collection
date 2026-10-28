// Startup intros skipped (ab_skip_intros, on by default; owner, 28 Oct 2026). Until the title screen first shows, the
// pad hook (input.cpp) taps A on user 0 every 0.4 s, which is how a player skips each logo and intro movie on the
// console. The title marks itself: TitleShown() is called from direct_start.cpp at the first call of the C function
// behind User_Button_Was_Released (the title waiting for Start; the logo screens use User_Button_Was_Pressed). A
// minute and a half after the first tap it stops in any case.

#include <atomic>
#include <chrono>
#include <cstdint>

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(ab_skip_intros, true, "Collection", "Skip the logos and intro movies at startup");

namespace abt {
namespace {
using Clock = std::chrono::steady_clock;
std::atomic<bool> g_title{false};
}  // namespace

void TitleShown() {
  if (!g_title.exchange(true) && REXCVAR_GET(ab_skip_intros)) REXLOG_INFO("ABT: title shown, intros skipped");
}

uint16_t SkipIntroButtons() {
  if (g_title.load() || !REXCVAR_GET(ab_skip_intros)) return 0;
  static const Clock::time_point start = Clock::now();
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
  if (ms > 90000) return 0;
  return ms % 400 < 100 ? 0x1000 : 0;  // XINPUT_GAMEPAD_A
}

}  // namespace abt
