// The port's defaults. They change only the default, so the settings file and the command line still win.
//
//   d3d12_submit_on_primary_buffer_end = false   The game asks for a frame on every vblank (present interval 1, 60 FPS)
//                                    but the main thread spent its frame waiting in the D3D swap for the GPU to drain
//                                    the ring buffer, and play ran at 45 to 50 FPS. The runtime sent the host GPU a
//                                    command list at the end of every guest ring buffer section, many small submissions
//                                    a frame. Sending them per frame instead (and whenever the runtime needs GPU results
//                                    back, which it still does on its own) holds 60 on the early screens (27 Oct 2026). Occlusion
//                                    queries, memexport readback and vsync were ruled out the same way.
//
// This runs from OnFinalizePaths (after the GPU plugin has registered its settings, before the GPU starts).

#include <string_view>

#include <rex/cvar.h>
#include <rex/logging.h>

namespace absw {
void ApplyFrameRateDefaults();  // frame_rate.cpp

void SetCvarDefault(std::string_view name, std::string_view value) {
  for (auto& e : rex::cvar::GetRegistry()) {
    if (e.name != name) continue;
    e.default_value = value;
    if (e.source == rex::cvar::Source::kDefault) e.setter(value);  // config and command line still win
    return;
  }
  REXLOG_WARN("ABSW: setting {} not found, its port default was not applied", name);
}

// Controller only (owner, 28 Oct 2026): the runtime's keyboard and mouse controller emulation stays off, also when
// a settings file asks for it.
void ApplyControllerOnly() { rex::cvar::SetFlagByName("mnk_mode", "false"); }

void ApplyPortDefaults() {
  SetCvarDefault("d3d12_submit_on_primary_buffer_end", "false");
  ApplyFrameRateDefaults();
  ApplyControllerOnly();
}

}  // namespace absw
