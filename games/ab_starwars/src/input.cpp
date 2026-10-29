// Controller input.
//
// sub_82595430 is the XDK's XInputGetState(user, state): it tail calls XamInputGetState(user, 1, state). Its two
// callers (sub_826FF440, sub_826FF170) read into stack buffers and skip a failed read, but a failed read still zeroes
// the state as it would on the console (playbook: "A failed pad read must still clear the state").
//
// Developer aid: ABSW_DEV_PAD="<seconds>:<input>[/<hold seconds>],..." presses inputs on user 0 at those times (seconds
// from the first pad read, held 0.2 s unless given), e.g. ABSW_DEV_PAD=10:start,14:a/2,14.5:lleft/1. Inputs: a b x y
// start back up down left right lb rb, and lleft lright lup ldown (left stick pushed all the way, or part of the way
// with *<fraction>, e.g. 66:ldown*0.5/2).

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <rex/hook.h>
#include <rex/logging.h>

namespace absw {
namespace {

struct Press {
  double at, hold;
  uint16_t buttons;
  float lx, ly;  // left stick, -1 to 1
};

uint16_t ButtonBits(const std::string& name) {
  static const struct { const char* name; uint16_t bits; } kButtons[] = {
      {"up", 0x0001}, {"down", 0x0002}, {"left", 0x0004}, {"right", 0x0008}, {"start", 0x0010},
      {"back", 0x0020}, {"lb", 0x0100}, {"rb", 0x0200}, {"a", 0x1000}, {"b", 0x2000}, {"x", 0x4000},
      {"y", 0x8000}};
  for (const auto& b : kButtons)
    if (name == b.name) return b.bits;
  return 0;
}

std::vector<Press> ParseScript() {
  std::vector<Press> presses;
#ifdef AB_DEV_TOOLS
  const char* env = std::getenv("ABSW_DEV_PAD");
#else
  const char* env = nullptr;  // release builds have no developer aids (CMakeLists.txt)
#endif
  if (!env) return presses;
  std::string s = env;
  size_t pos = 0;
  while (pos < s.size()) {
    size_t end = s.find(',', pos);
    if (end == std::string::npos) end = s.size();
    const std::string item = s.substr(pos, end - pos);
    if (const size_t colon = item.find(':'); colon != std::string::npos) {
      std::string name = item.substr(colon + 1);
      double hold = 0.2;
      if (const size_t slash = name.find('/'); slash != std::string::npos) {
        hold = std::atof(name.substr(slash + 1).c_str());
        name = name.substr(0, slash);
      }
      float amount = 1.0f;
      if (const size_t star = name.find('*'); star != std::string::npos) {
        amount = float(std::atof(name.substr(star + 1).c_str()));
        name = name.substr(0, star);
      }
      const float lx = name == "lleft" ? -amount : name == "lright" ? amount : 0.0f;
      const float ly = name == "ldown" ? -amount : name == "lup" ? amount : 0.0f;
      presses.push_back({std::atof(item.substr(0, colon).c_str()), hold, ButtonBits(name), lx, ly});
    }
    pos = end + 1;
  }
  return presses;
}

const std::vector<Press> g_script = ParseScript();
std::atomic<uint64_t> g_reads{0};

struct Scripted {
  bool active = false;
  uint16_t buttons = 0;
  float lx = 0.0f, ly = 0.0f;
};

// What the script holds at this moment.
Scripted ScriptedInput() {
  Scripted in;
  if (g_script.empty()) return in;
  using Clock = std::chrono::steady_clock;
  static const Clock::time_point start = Clock::now();
  const double t = std::chrono::duration<double>(Clock::now() - start).count();
  for (const Press& p : g_script) {
    if (t < p.at || t >= p.at + p.hold) continue;
    in.active = true;
    in.buttons |= p.buttons;
    if (p.lx) in.lx = p.lx;
    if (p.ly) in.ly = p.ly;
  }
  return in;
}

void PutS16(uint8_t* p, int v) {
  p[0] = uint8_t(uint16_t(v) >> 8);
  p[1] = uint8_t(v);
}

}  // namespace

uint64_t PadReads() { return g_reads.load(); }
void ReturnToLauncherTick(uint32_t user, bool connected, uint16_t buttons);  // return_to_launcher.cpp
void GameWindowTick(bool pad_used);                                          // game_window.cpp
bool PadChanged(uint32_t user, const uint8_t* state16);                      // game_window.cpp
uint16_t SkipIntroButtons();  // skip_intros.cpp
bool Scripting() { return !g_script.empty(); }
uint16_t ScriptedButtonsNow() { return ScriptedInput().buttons; }  // for dialog_pad.cpp

}  // namespace absw

REX_EXTERN(__imp__sub_82595430);
REX_HOOK_RAW(sub_82595430) {
  const uint32_t user = ctx.r3.u32, state_ptr = ctx.r4.u32;
  __imp__sub_82595430(ctx, base);
  ++absw::g_reads;
  if (ctx.r3.u32 != 0 && state_ptr) std::memset(base + state_ptr, 0, 16);  // not ERROR_SUCCESS
  if (user == 0 && state_ptr && absw::Scripting()) {
    ctx.r3.u64 = 0;  // the scripted pad is always connected (the state is zeroed above when no real pad answered)
    if (const absw::Scripted in = absw::ScriptedInput(); in.active) {
      // XINPUT_STATE, big endian: dwPacketNumber, then XINPUT_GAMEPAD wButtons +4, triggers +6/+7, sThumbLX +8, LY +10.
      uint8_t* s = base + state_ptr;
      const uint16_t buttons = uint16_t(s[4] << 8 | s[5]) | in.buttons;
      s[4] = uint8_t(buttons >> 8);
      s[5] = uint8_t(buttons);
      if (in.lx != 0.0f) absw::PutS16(s + 8, int(in.lx * 32767));
      if (in.ly != 0.0f) absw::PutS16(s + 10, int(in.ly * 32767));
    }
  }
  if (user == 0 && state_ptr) {  // the startup intros skipped (skip_intros.cpp)
    if (const uint16_t skip = absw::SkipIntroButtons()) {
      uint8_t* s = base + state_ptr;
      const uint16_t buttons = uint16_t(s[4] << 8 | s[5]) | skip;
      s[4] = uint8_t(buttons >> 8);
      s[5] = uint8_t(buttons);
      ctx.r3.u64 = 0;
    }
  }
  // Collection launcher (TRG Launcher 2.0): Back and Start held returns to it; its window comes to the front.
  const bool connected = ctx.r3.u32 == 0 && state_ptr;
  absw::ReturnToLauncherTick(user, connected, connected ? uint16_t(base[state_ptr + 4] << 8 | base[state_ptr + 5]) : 0);
  absw::GameWindowTick(connected && absw::PadChanged(user, base + state_ptr));
}

// sub_82595428 is XInputGetCapabilities(user, flags, caps) (a tail call to XamInputGetCapabilities). The game reads a
// pad's state only when this says it is connected, so the scripted pad answers here too: a gamepad on user 0.
REX_EXTERN(__imp__sub_82595428);
REX_HOOK_RAW(sub_82595428) {
  const uint32_t user = ctx.r3.u32, caps = ctx.r5.u32;
  __imp__sub_82595428(ctx, base);
  if (user == 0 && caps && ctx.r3.u32 != 0 && absw::Scripting()) {
    std::memset(base + caps, 0, 20);  // XINPUT_CAPABILITIES: Type, SubType, Flags, Gamepad, Vibration
    base[caps] = 1;                   // XINPUT_DEVTYPE_GAMEPAD
    base[caps + 1] = 1;               // XINPUT_DEVSUBTYPE_GAMEPAD
    ctx.r3.u64 = 0;
  }
}
