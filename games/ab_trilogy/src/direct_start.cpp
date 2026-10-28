// Direct start into one of the three games (for the launcher): --abt_start_game=classic, rio or seasons.
//
// Angry Birds Trilogy is three games behind one menu. After the logos the title waits for Start, then the main menu
// wheel (screen_main.lua, Screen_Main) offers Seasons, Classic and Rio; picking one runs the menu's local
// To_Episode_Select(game, item): Stream_GC(), Initialize_Game(game), then screen:Push_Screen(Screen_Episode_Select(false,
// 1), Scroll_Transition_Right("menu_episode_select")). The game's own -game: option only works in debug builds with
// -level:, so this does what the player would do, through the game's Lua:
//   * The title calls User_Button_Was_Released({BUTTON_START, BUTTON_ACCEPT}) every frame and goes on with the user it
//     returns. While it waits, ABT_PRESS_START is set and the pad hook (input.cpp) presses Start on user 0 in short
//     taps, as the player would.
//   * The wheel's on_enter calls Check_For_DLC(screen) on its first entry (only screen_main.lua uses it; it pushes the
//     DLC screen and returns true when there is DLC to check). Right after that, on its first call (the wheel's first entry),
//     the menu's own game steps run for the chosen game. Back from the episode select returns to the wheel as usual.
//   * Without Xbox LIVE the wheel's Perform_Leaderboard_Submit_Check shows Do_Error(screen,
//     L10N.error_online_leaderboards_submit_sign_in_xb, callback) once ("In order to submit scores to the leaderboards,
//     you must be connected to Xbox LIVE"); its OK only runs the callback, screen:Pop_Screen(""). The direct start
//     answers it for the player: the first time, that notice is not opened (owner decision, 28 Oct 2026).
// All are wrappers around the game's globals, installed once from the first call of the C function behind
// User_Button_Was_Released (sub_82567108, which gives the Lua state) after the menu scripts have loaded. A Lua error
// raised under a host hook does not come back to the game, so nothing here raises one.
//
// Guest Lua 5.1 functions used: luaL_loadstring sub_825895D8, lua_pcall sub_825881C8, lua_getfield sub_82587AC0,
// lua_type sub_82586FB0, lua_settop sub_82586CF8, lua_tolstring sub_825873D8 (found from run_script, sub_8258AD88, and
// the order of lapi.c). Scripts were read with ABT_DEV_FIND and tools\luadis.py.

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>

REXCVAR_DEFINE_STRING(abt_start_game, "", "Angry Birds Trilogy",
                      "Start straight into one game: classic, rio or seasons (empty: the Trilogy menu)");

REX_EXTERN(sub_825895D8);  // luaL_loadstring(L, s)
REX_EXTERN(sub_825881C8);  // lua_pcall(L, nargs, nresults, errfunc)
REX_EXTERN(sub_82587AC0);  // lua_getfield(L, index, k)
REX_EXTERN(sub_82586FB0);  // lua_type(L, index)
REX_EXTERN(sub_82586CF8);  // lua_settop(L, index)
REX_EXTERN(sub_825873D8);  // lua_tolstring(L, index, len)

namespace abt {
namespace {

constexpr int32_t kLuaGlobals = -10002;  // LUA_GLOBALSINDEX

const char* GameGlobal(const std::string& name) {
  if (name == "classic") return "GAME_ORIGINAL";
  if (name == "rio") return "GAME_RIO";
  if (name == "seasons") return "GAME_SEASONS";
  return nullptr;
}

std::string InstallScript(const char* game) {
  return std::string(R"lua(
if ABT_DIRECT_GAME == nil then
  local game = )lua") + game + R"lua(
  if game == nil or Check_For_DLC == nil or Initialize_Game == nil or Screen_Episode_Select == nil
      or Scroll_Transition_Right == nil or Stream_GC == nil or Do_Error == nil or L10N == nil then
    return  -- the menu scripts are not loaded yet
  end
  ABT_DIRECT_GAME = game
  local title_waiting = true
  local released = User_Button_Was_Released
  User_Button_Was_Released = function(buttons, ...)
    local user = released(buttons, ...)
    if title_waiting and type(buttons) == "table" and buttons[1] == BUTTON_START and buttons[2] == BUTTON_ACCEPT then
      if user ~= nil then title_waiting = false end
      if title_waiting then ABT_PRESS_START = true else ABT_PRESS_START = nil end
    end
    return user
  end
  local do_error = Do_Error
  local notice_waiting = true
  Do_Error = function(screen, text, ...)
    if notice_waiting and text ~= nil and text == L10N.error_online_leaderboards_submit_sign_in_xb then
      notice_waiting = false
      return  -- OK would only close it: its callback is screen:Pop_Screen("")
    end
    return do_error(screen, text, ...)
  end
  local check = Check_For_DLC
  local wheel_waiting = true
  Check_For_DLC = function(screen, ...)
    local pushed = check(screen, ...)
    if wheel_waiting and not pushed and type(screen) == "table" then
      wheel_waiting = false
      Stream_GC()
      Initialize_Game(game)
      screen:Push_Screen(Screen_Episode_Select(false, 1), Scroll_Transition_Right("menu_episode_select"))
    end
    return pushed
  end
end
)lua";
}

std::atomic<int64_t> g_press_until_ms{0};  // the title asked for Start until then (renewed each title frame)

int64_t NowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
bool g_installed = false;
int g_install_tries = 0;

// Runs guest calls on a scratch area below the caller's stack, then puts every register back.
class GuestCalls {
 public:
  GuestCalls(PPCContext& ctx, uint8_t* base)
      : ctx_(ctx), base_(base), saved_(ctx), next_((ctx.r1.u32 - 0x100) & ~0xFu) {}  // past the caller's red zone
  ~GuestCalls() { ctx_ = saved_; }

  uint32_t String(const std::string& s) {
    next_ = (next_ - uint32_t(s.size()) - 1) & ~0xFu;
    std::memcpy(base_ + next_, s.c_str(), s.size() + 1);
    return next_;
  }

  uint32_t Call(void (*fn)(PPCContext&, uint8_t*), uint32_t a, uint32_t b = 0, uint32_t c = 0, uint32_t d = 0) {
    ctx_.r1.u64 = (next_ - 0x200) & ~0xFull;  // callees save into the bytes below their r1
    ctx_.r3.u64 = a;
    ctx_.r4.s64 = int32_t(b);
    ctx_.r5.s64 = int32_t(c);
    ctx_.r6.s64 = int32_t(d);
    fn(ctx_, base_);
    return ctx_.r3.u32;
  }

 private:
  PPCContext& ctx_;
  uint8_t* base_;
  PPCContext saved_;
  uint32_t next_;
};

// The type of a Lua global (lua_type: -1 none, 0 nil, 1 boolean, 4 string, ...).
int32_t Global(PPCContext& ctx, uint8_t* base, uint32_t L, const char* name) {
  GuestCalls g(ctx, base);
  g.Call(sub_82587AC0, L, uint32_t(kLuaGlobals), g.String(name));
  const int32_t type = int32_t(g.Call(sub_82586FB0, L, uint32_t(-1)));
  g.Call(sub_82586CF8, L, uint32_t(-2));  // pop it
  return type;
}

// Runs the install script. It never raises a Lua error (a guest error raised under a host hook does not come back), it
// just does nothing until the menu scripts are there.
void Install(PPCContext& ctx, uint8_t* base, uint32_t L, const char* game) {
  {
    GuestCalls g(ctx, base);
    const uint32_t src = g.String(InstallScript(game));
    if (g.Call(sub_825895D8, L, src) != 0) {
      const uint32_t p = g.Call(sub_825873D8, L, uint32_t(-1));
      REXLOG_ERROR("ABT: direct start script did not compile: {}", p ? reinterpret_cast<const char*>(base + p) : "?");
      g.Call(sub_82586CF8, L, uint32_t(-2));
      g_install_tries = -1;  // do not try again
      return;
    }
    g.Call(sub_825881C8, L, 0, 0, 0);
  }
  if (Global(ctx, base, L, "ABT_DIRECT_GAME") > 0) {
    g_installed = true;
    REXLOG_INFO("ABT: direct start into {} is set up", REXCVAR_GET(abt_start_game));
  } else if (++g_install_tries == 1) {
    REXLOG_INFO("ABT: direct start waits for the menu scripts");
  }
}

}  // namespace

// Buttons the direct start presses on user 0 right now (for input.cpp): Start in 0.15 s taps every 0.6 s while the title
// waits for it (each title frame asks again; the taps end half a second after it stops).
void TitleShown();  // skip_intros.cpp

uint16_t DirectStartButtons() {
  const int64_t ms = NowMs();
  if (ms >= g_press_until_ms.load()) return 0;
  return ms % 600 < 150 ? 0x0010 : 0;  // XINPUT_GAMEPAD_START
}

void DirectStartFrame(PPCContext& ctx, uint8_t* base) {
  static bool title_done = false;
  const char* game = GameGlobal(REXCVAR_GET(abt_start_game));
  if (!game || title_done) return;
  const uint32_t L = ctx.r3.u32;
  if (!g_installed && g_install_tries >= 0) Install(ctx, base, L, game);
  if (!g_installed) return;
  static bool pressing = false;
  if (Global(ctx, base, L, "ABT_PRESS_START") > 0) {
    g_press_until_ms = NowMs() + 500;  // stops on its own once the title stops asking
    if (!pressing) REXLOG_INFO("ABT: direct start is pressing Start for the title");
    pressing = true;
  } else if (pressing) {
    title_done = true;  // the rest happens in the game's Lua
  }
}

}  // namespace abt

// sub_82567108: the C function behind the Lua global User_Button_Was_Released (r3 = lua_State; the title calls it).
REX_EXTERN(__imp__sub_82567108);
REX_HOOK_RAW(sub_82567108) {
  abt::TitleShown();  // skip_intros.cpp: the logos are over
  abt::DirectStartFrame(ctx, base);
  __imp__sub_82567108(ctx, base);
}
