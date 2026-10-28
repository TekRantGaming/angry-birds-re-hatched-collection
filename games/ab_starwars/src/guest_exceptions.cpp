// Guest exceptions and bug checks, logged before the runtime stops the game. RtlRaiseException(record) comes from the
// game's C++ throw (code E06D7363) and KeBugCheck(Ex) from its fatal errors; the runtime's versions can only break
// into a debugger (the recompiled code cannot unwind to a catch), which ends the process with 0x80000003. These
// versions say what was thrown and from where (the caller's address), then do the same.

#include <cstdint>
#include <string>

#include <rex/hook.h>
#include <rex/logging.h>

#if defined(_WIN32)
#include <intrin.h>
#endif

namespace {

uint32_t U32(const uint8_t* base, uint32_t a) {
  return uint32_t(base[a]) << 24 | uint32_t(base[a + 1]) << 16 | uint32_t(base[a + 2]) << 8 | base[a + 3];
}

std::string CString(const uint8_t* base, uint32_t a, size_t limit = 120) {
  std::string s;
  for (size_t i = 0; a && i < limit && base[a + i]; ++i) s += char(base[a + i]);
  return s;
}

[[noreturn]] void Stop() {
#if defined(_WIN32)
  __debugbreak();
#endif
  std::abort();
}

}  // namespace

// EXCEPTION_RECORD (big endian): code +0, flags +4, record +8, address +12, parameter count +16, parameters +20. A C++
// throw has 3 parameters: magic, the thrown object, its ThrowInfo (attributes, unwind, forward, CatchableTypeArray);
// the array holds a count and CatchableType pointers (properties, TypeDescriptor, ...); a TypeDescriptor's name
// (".?AV...@@") starts at +8.
extern "C" REX_FUNC(__imp__RtlRaiseException) {
  const uint32_t record = ctx.r3.u32;
  const uint32_t code = U32(base, record);
  std::string what;
  if (code == 0xE06D7363 && U32(base, record + 16) == 3) {
    const uint32_t throw_info = U32(base, record + 28);
    const uint32_t types = throw_info ? U32(base, throw_info + 12) : 0;
    const uint32_t count = types ? U32(base, types) : 0;
    for (uint32_t i = 0; i < count && i < 4; ++i) {
      const uint32_t type = U32(base, types + 4 + 4 * i);
      const uint32_t descriptor = type ? U32(base, type + 4) : 0;
      what += " " + CString(base, descriptor + 8);
    }
  }
  if (code == 0x406D1388) return;  // SetThreadName: nothing to do
  // The callers: each frame's back chain is at its r1, and the code saves its return address 8 bytes below the back
  // chain it points to.
  std::string callers;
  for (uint32_t frame = ctx.r1.u32, i = 0; frame && i < 10; ++i) {
    const uint32_t back = U32(base, frame);
    if (!back || back <= frame) break;
    callers += fmt::format(" {:08X}", U32(base, back - 8));
    frame = back;
  }
  // Text the thrown object points to (its message), for the log.
  std::string text;
  if (code == 0xE06D7363) {
    const uint32_t object = U32(base, record + 24);
    for (uint32_t off = 0; object && off < 0x40; off += 4) {
      const uint32_t p = U32(base, object + off);
      if (p < 0x40000000 || p >= 0xA0000000) continue;
      const std::string narrow = CString(base, p);
      if (narrow.size() > 3) text += " '" + narrow + "'";
      std::string wide;  // big endian UTF-16
      for (uint32_t i = 0; i < 120 && base[p + 2 * i + 1] && !base[p + 2 * i]; ++i) wide += char(base[p + 2 * i + 1]);
      if (wide.size() > 3) text += " '" + wide + "'";
    }
  }
  REXLOG_ERROR("ABSW: guest exception {:08X} from {:08X} (thrown:{};{}), callers{}", code, uint32_t(ctx.lr), what, text,
               callers);
  Stop();
}

extern "C" REX_FUNC(__imp__KeBugCheckEx) {
  (void)base;
  REXLOG_ERROR("ABSW: guest bug check {:08X} ({:08X}, {:08X}, {:08X}, {:08X}) from {:08X}", ctx.r3.u32, ctx.r4.u32,
               ctx.r5.u32, ctx.r6.u32, ctx.r7.u32, uint32_t(ctx.lr));
  Stop();
}

extern "C" REX_FUNC(__imp__KeBugCheck) {
  (void)base;
  REXLOG_ERROR("ABSW: guest bug check {:08X} from {:08X}", ctx.r3.u32, uint32_t(ctx.lr));
  Stop();
}
