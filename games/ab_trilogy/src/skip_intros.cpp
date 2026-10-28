// Startup intros skipped (ab_skip_intros, on by default; owner, 28 Oct 2026).
//
// The logos are the menu script Screen_Splash (screen_splash.lua). Its routine waits for streaming (Wait_Condition with
// Is_Streaming), starts the audio, shows Rovio, Activision, Fox and Blue Sky and the autosave notice through its local
// Fade_In_Out (the globals Items_Fade, then Wait), waits for the audio to load and then replaces itself with
// Screen_Title. So at the first call of Is_Streaming (sub_825691D0, before anything is shown) the script below makes
// Items_Fade and Wait do nothing until Screen_Title is called: the items stay transparent, the loading still happens,
// and the title comes as soon as the game is ready. (The game's -nosplash option only reaches the scripts as a flag
// they do not read.)
//
// As a fallback, until the title first shows, the pad hook (input.cpp) taps A on user 0 every 0.4 s, which is how a
// player skips each logo. The title marks itself: TitleShown() is called from direct_start.cpp at the first call of the
// C function behind User_Button_Was_Released (the title waiting for Start; the logo screens use
// User_Button_Was_Pressed). A minute and a half after the first tap it stops in any case.

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

#include <rex/cvar.h>
#include <rex/hook.h>
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

namespace abt {
bool RunLuaChunk(PPCContext& ctx, uint8_t* base, uint32_t L, const std::string& source);  // direct_start.cpp
int32_t LuaGlobalType(PPCContext& ctx, uint8_t* base, uint32_t L, const char* name);     // direct_start.cpp
}  // namespace abt

// sub_825691D0: the C function behind the Lua global Is_Streaming (r3 = lua_State).
REX_EXTERN(__imp__sub_825691D0);
REX_HOOK_RAW(sub_825691D0) {
  static bool done = false;
  if (!done && REXCVAR_GET(ab_skip_intros)) {
    const uint32_t L = ctx.r3.u32;
    static const std::string kScript = R"lua(
if ABT_SPLASH_SKIP == nil and type(Items_Fade) == "function" and type(Wait) == "function"
    and type(Screen_Title) == "function" then
  ABT_SPLASH_SKIP = true
  local fade, wait, title = Items_Fade, Wait, Screen_Title
  Items_Fade = function() end
  Wait = function() end
  Screen_Title = function(...)
    Items_Fade, Wait, Screen_Title = fade, wait, title
    return title(...)
  end
end
)lua";
    if (!abt::RunLuaChunk(ctx, base, L, kScript)) {
      REXLOG_ERROR("ABT: splash skip script did not compile");
      done = true;
    } else if (abt::LuaGlobalType(ctx, base, L, "ABT_SPLASH_SKIP") > 0) {
      REXLOG_INFO("ABT: splash logos skipped");
      done = true;
    }
  }
  __imp__sub_825691D0(ctx, base);
}
