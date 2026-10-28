// Frame counter: counts the game's own frames (sub_82396270 is the D3D swap, the one function that calls VdSwap) for
// the SDK's F3 overlay and, with absw_log_fps on, a line in the log each second (frames, average and slowest frame).
// Adapted from the Burger King collection (games/sneak_king/src/frame_stats.cpp).

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/ui/overlay/debug_overlay.h>

REXCVAR_DEFINE_BOOL(absw_log_fps, false, "Angry Birds Star Wars", "Log the game's frame rate once a second");

namespace absw {
uint64_t PadReads();  // input.cpp

namespace {

using Clock = std::chrono::steady_clock;

struct Window {
  Clock::time_point start = Clock::now(), last = start;
  uint32_t frames = 0;
  double slowest_ms = 0.0;
};
Window g_window;  // only the game's render thread presents
std::atomic<uint64_t> g_total{0};
std::atomic<double> g_fps{0.0}, g_frame_ms{0.0};

void CountFrame() {
  const Clock::time_point now = Clock::now();
  Window& w = g_window;
  w.slowest_ms = std::max(w.slowest_ms, std::chrono::duration<double, std::milli>(now - w.last).count());
  w.last = now;
  ++w.frames;
  const uint64_t total = ++g_total;
  if (total == 1) REXLOG_INFO("ABSW: first frame presented");
  const double secs = std::chrono::duration<double>(now - w.start).count();
  if (secs < 1.0) return;
  g_fps = w.frames / secs;
  g_frame_ms = secs * 1000.0 / w.frames;
  if (REXCVAR_GET(absw_log_fps)) {
    REXLOG_INFO("ABSW fps: {:.1f} ({} frames, {:.2f} ms average, slowest {:.2f} ms), pad reads {}", g_fps.load(),
                w.frames, g_frame_ms.load(), w.slowest_ms, PadReads());
  }
  w = Window();
  w.start = w.last = now;
}

}  // namespace

rex::ui::FrameStats GuestFrameStats() {
  rex::ui::FrameStats s;
  s.fps = g_fps;
  s.frame_time_ms = g_frame_ms;
  s.frame_count = g_total;
  return s;
}

}  // namespace absw

REX_EXTERN(__imp__sub_82396270);
REX_HOOK_RAW(sub_82396270) {
  __imp__sub_82396270(ctx, base);
  absw::CountFrame();
}
