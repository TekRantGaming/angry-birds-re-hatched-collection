// Frame rate (ab_frame_rate, set by the collection launcher): 30, 60, 90, 120, or 0 for no limit.
//
// The game asks for a frame on every guest vertical blank (present interval 1) and steps its world by the real time
// that passed, so the vblank rate is its frame rate: 30, 90 and 120 set it to that, no limit to the runtime's highest,
// 240 Hz. Checked at 120 (27 Oct 2026): bird flight and camera at the same speed as at 60, level 1-1 cleared the same way.

#include <string>
#include <string_view>

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_INT32(ab_frame_rate, 60, "Angry Birds Star Wars", "Frame rate: 30, 60, 90, 120, or 0 for no limit");

namespace absw {
void SetCvarDefault(std::string_view name, std::string_view value);  // port_defaults.cpp

// From ApplyPortDefaults (before the GPU starts).
void ApplyFrameRateDefaults() {
  const int rate = REXCVAR_GET(ab_frame_rate);
  if (rate == 30 || rate == 90 || rate == 120 || rate == 0)
    SetCvarDefault("video_mode_refresh_rate", std::to_string(rate ? rate : 240));
  if (rate != 60) REXLOG_INFO("ABSW: frame rate {}", rate ? std::to_string(rate) : std::string("without a limit"));
}

}  // namespace absw
