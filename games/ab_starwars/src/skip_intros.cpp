// Startup intros skipped (ab_skip_intros, on by default; owner, 28 Oct 2026). Until the title screen first shows, the
// pad hook (input.cpp) taps A on user 0 every 0.4 s, which is how a player skips each logo and intro movie on the
// console. The title marks itself: TitleShown() is called from no_multiplayer.cpp when the game switches to the
// screen MainMenu ("Press Start to play"). A minute and a half after the first tap it stops in any case. Star Wars' title screen then gets one more tap (below).

#include <atomic>
#include <chrono>
#include <cstdint>

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(ab_skip_intros, true, "Collection", "Skip the logos and intro movies at startup");

namespace absw {
namespace {
using Clock = std::chrono::steady_clock;
std::atomic<bool> g_title{false};
std::atomic<int64_t> g_title_ms{0};  // when MainMenu opened

int64_t NowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
}
}  // namespace

void TitleShown() {
  g_title_ms = NowMs();
  if (!g_title.exchange(true) && REXCVAR_GET(ab_skip_intros)) REXLOG_INFO("ABSW: title shown, intros skipped");
}

uint16_t SkipIntroButtons() {
  if (!REXCVAR_GET(ab_skip_intros)) return 0;
  if (g_title.load()) {
    // MainMenu opens with "A long time ago in a galaxy far, far away" before "Press Start to play": one more A
    // 2.4 s in skips that text and stops at the title (a second A would go on to Play Mode Selection).
    static std::atomic<bool> done{false};
    const int64_t since = NowMs() - g_title_ms.load();
    if (since >= 2400 && since < 2500 && !done) return 0x1000;
    if (since >= 2500) done = true;
    return 0;
  }
  static const Clock::time_point start = Clock::now();
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
  if (ms > 90000) return 0;
  return ms % 400 < 100 ? 0x1000 : 0;  // XINPUT_GAMEPAD_A
}

}  // namespace absw
