// Frame rate (ab_frame_rate, set by the collection launcher): 30, 60 (the game's own), 90, 120, or 0 for no limit.
//
// Two things hold the game at 60:
//  * the guest vertical blank (60 Hz): the D3D swap holds each frame until the vblanks of the game's present interval
//    (1) have passed;
//  * its main loop, sub_8255F8D0(f1, step, min_frame, max_time), called once a frame from sub_8255FAD8 with values from
//    its config: the game logic runs in fixed steps of `step` seconds (1/60; up to five a frame, so a slow frame
//    catches up) and a frame never ends before `min_frame` (1/60) has passed.
// 30 only lowers the vblank to 30 Hz: two logic steps a frame keep the game at its speed (checked 28 Oct 2026). 90 and
// 120 raise the vblank and make the step and the minimum frame time one frame at that rate, so every frame shows a
// new logic step. No limit sets the vblank to the runtime's highest, 240 Hz, drops the minimum frame time and steps the
// logic at 1/120 s. The game measures real time for its step count, so it runs at the same speed at every setting.
// Limits found on the way (28 Oct 2026): the loop runs at most five steps a frame and drops the rest of the time, so
// a step of 1/240 s lost time on every frame longer than 21 ms (logos, story videos and loading ran slowly); turning the
// guest vblank off (1000 Hz) slowed the videos as well. The catch up limit (max_time, 8/60 s) is left as it is.

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>

REXCVAR_DEFINE_INT32(ab_frame_rate, 60, "Angry Birds Trilogy", "Frame rate: 30, 60, 90, 120, or 0 for no limit");

namespace abt {
void SetCvarDefault(std::string_view name, std::string_view value);  // port_defaults.cpp

namespace {
// Seconds per logic step and minimum seconds per frame for the setting (0 keeps the game's own).
double StepFor(int rate) { return rate == 90 || rate == 120 ? 1.0 / rate : rate == 0 ? 1.0 / 120 : 0.0; }
double MinFrameFor(int rate) { return rate == 90 || rate == 120 ? 1.0 / rate : 0.0; }
}  // namespace

// From ApplyPortDefaults (before the GPU starts): the guest vblank for the setting.
void ApplyFrameRateDefaults() {
  const int rate = REXCVAR_GET(ab_frame_rate);
  if (rate == 30 || rate == 90 || rate == 120 || rate == 0)
    SetCvarDefault("video_mode_refresh_rate", std::to_string(rate ? rate : 240));
  if (rate != 60) REXLOG_INFO("ABT: frame rate {}", rate ? std::to_string(rate) : std::string("without a limit"));
}

}  // namespace abt

REX_EXTERN(__imp__sub_8255F8D0);
REX_HOOK_RAW(sub_8255F8D0) {
  const int rate = REXCVAR_GET(ab_frame_rate);
  if (const double step = abt::StepFor(rate); step > 0.0) {
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true))
      REXLOG_INFO("ABT: game loop step {:.5f} s, minimum frame {:.5f} s, catch up {:.5f} s; now step {:.5f} s",
                  ctx.f2.f64, ctx.f3.f64, ctx.f4.f64, step);
    ctx.f2.f64 = step;
    ctx.f3.f64 = abt::MinFrameFor(rate);
  }
  __imp__sub_8255F8D0(ctx, base);
}
