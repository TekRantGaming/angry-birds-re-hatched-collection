// The port's defaults. They change only the default, so the settings file and the command line still win.
//
//   render_target_path_d3d12 = rov   Every sprite of the logos, title and menus is drawn with a depth test, after the
//                                    game clears depth with a draw on a 4x MSAA surface at half the pitch that shares
//                                    the same EDRAM. The host render target path (rtv) does not carry that clear over
//                                    to the 1x surface the sprites use, so they all fail the test and the screen stays
//                                    black after the Rovio logo (its quad draws with depth off). The ROV path emulates
//                                    EDRAM exactly and shows everything (27 Oct 2026).
//
// The GPU reads its render target path when it starts, so this runs from OnFinalizePaths (after the GPU plugin has
// registered its settings, before the GPU starts).

#include <string_view>

#include <rex/cvar.h>
#include <rex/logging.h>

namespace abt {
void ApplyFrameRateDefaults();  // frame_rate.cpp

void SetCvarDefault(std::string_view name, std::string_view value) {
  for (auto& e : rex::cvar::GetRegistry()) {
    if (e.name != name) continue;
    e.default_value = value;
    if (e.source == rex::cvar::Source::kDefault) e.setter(value);  // config and command line still win
    return;
  }
  REXLOG_WARN("ABT: setting {} not found, its port default was not applied", name);
}

// Controller only (owner, 28 Oct 2026): the runtime's keyboard and mouse controller emulation stays off, also when
// a settings file asks for it.
void ApplyControllerOnly() { rex::cvar::SetFlagByName("mnk_mode", "false"); }

void ApplyPortDefaults() {
  SetCvarDefault("render_target_path_d3d12", "rov");
  ApplyFrameRateDefaults();
  ApplyControllerOnly();
}

}  // namespace abt
