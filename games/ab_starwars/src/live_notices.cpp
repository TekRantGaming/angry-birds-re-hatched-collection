// Xbox LIVE notices. sub_8258DBA8 is XShowMessageBoxUI(user, title, text, button count, buttons, focus button, flags,
// result, overlapped); the strings are big endian UTF-16. After Start on the title screen the game opens "Not
// Connected: In order to view the leaderboards, you must be connected to Xbox LIVE" with one OK button, and it can
// never be connected on PC. The owner never wants to see it (28 Oct 2026): a box whose text names Xbox LIVE is answered
// at once with its focused button, as the runtime does with its `headless` setting, which is turned on for that one
// call. Every other box still shows (and the controller answers it, dialog_pad.cpp). Each box is logged.

#include <cstdint>
#include <string>
#include <string_view>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>

namespace {

std::string GuestWide(const uint8_t* base, uint32_t ptr, size_t limit = 400) {
  std::string s;
  if (!ptr) return s;
  for (size_t i = 0; i < limit; ++i) {
    const uint16_t c = uint16_t(base[ptr + 2 * i] << 8 | base[ptr + 2 * i + 1]);
    if (!c) break;
    s += c < 128 ? char(c) : '?';
  }
  return s;
}

uint32_t GuestU32(const uint8_t* base, uint32_t a) {
  return uint32_t(base[a]) << 24 | uint32_t(base[a + 1]) << 16 | uint32_t(base[a + 2]) << 8 | base[a + 3];
}

// The runtime's `headless` setting (defined in the runtime, so set through the registry).
void SetHeadless(bool on) {
  for (auto& e : rex::cvar::GetRegistry())
    if (e.name == "headless") e.setter(on ? "true" : "false");
}

}  // namespace

REX_EXTERN(__imp__sub_8258DBA8);
REX_HOOK_RAW(sub_8258DBA8) {
  std::string buttons;
  for (uint32_t i = 0; i < ctx.r6.u32 && i < 3; ++i) buttons += "[" + GuestWide(base, GuestU32(base, ctx.r7.u32 + 4 * i)) + "] ";
  const std::string text = GuestWide(base, ctx.r5.u32);
  const bool live = text.find("Xbox LIVE") != std::string::npos;
  REXLOG_INFO("ABSW: message box title '{}' text '{}' buttons {}focus {} flags {:X}{}", GuestWide(base, ctx.r4.u32), text,
              buttons, ctx.r8.u32, ctx.r9.u32, live ? ", answered without showing it" : "");
  if (live) SetHeadless(true);
  __imp__sub_8258DBA8(ctx, base);
  if (live) SetHeadless(false);
}
