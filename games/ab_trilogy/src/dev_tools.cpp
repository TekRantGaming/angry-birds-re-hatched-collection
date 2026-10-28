// Developer aids, each turned on by an environment variable and off otherwise:
//   ABT_DUMP_IMAGE=<file>   writes the decrypted guest image (for the tools in tools\: function finding, disassembly).
//   ABT_DEV_SHOTS=<folder>  saves the game's own frame as shotN.bmp every 2 seconds, 30 times (ABT_DEV_SHOTS_MS and
//                           ABT_DEV_SHOTS_COUNT change the interval and count, ABT_DEV_SHOTS_START waits that many
//                           seconds first). From the Burger King collection.
//   ABT_DEV_AUDIO=1         logs the loudest sample each second in the mixer's output frame (guest 0x8275C790, the
//                           samples pointer the game passes to XAudioSubmitRenderDriverFrame: 6 x 256 big endian floats).
//   ABT_DEV_FIND=<text>     after ABT_DEV_FIND_AT seconds (default 30) searches guest memory for the text and saves
//                           64 KB around each hit (up to 40) as find_<address>.bin in the working folder.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#define NOMINMAX
#include <windows.h>

#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/interfaces/graphics.h>
#include <rex/ui/presenter.h>
#include <rex/ui/windowed_app_context.h>

namespace abt {
namespace {

void SaveBmp(const rex::ui::RawImage& img, const std::filesystem::path& path) {
  uint32_t row = img.width * 3, pad = (4 - row % 4) % 4, size = (row + pad) * img.height;
  uint8_t hdr[54] = {'B', 'M'};
  auto put = [&](int o, uint32_t v) {
    for (int k = 0; k < 4; ++k) hdr[o + k] = (v >> (8 * k)) & 255;
  };
  put(2, 54 + size);
  put(10, 54);
  put(14, 40);
  put(18, img.width);
  put(22, img.height);
  hdr[26] = 1;
  hdr[28] = 24;
  put(34, size);
  std::ofstream f(path, std::ios::binary);
  f.write(reinterpret_cast<char*>(hdr), 54);
  std::vector<uint8_t> line(row + pad, 0);
  for (int y = int(img.height) - 1; y >= 0; --y) {
    const uint8_t* s = img.data.data() + size_t(y) * img.stride;
    for (uint32_t x = 0; x < img.width; ++x) {
      line[x * 3] = s[x * 4 + 2];
      line[x * 3 + 1] = s[x * 4 + 1];
      line[x * 3 + 2] = s[x * 4];
    }
    f.write(reinterpret_cast<char*>(line.data()), line.size());
  }
}

void StartShots(rex::Runtime* runtime, rex::ui::WindowedAppContext& app_context, std::filesystem::path dir) {
  const char* ms_env = std::getenv("ABT_DEV_SHOTS_MS");
  const char* count_env = std::getenv("ABT_DEV_SHOTS_COUNT");
  const int interval_ms = ms_env ? std::atoi(ms_env) : 2000;
  const int count = count_env ? std::atoi(count_env) : 30;
  const char* start_env = std::getenv("ABT_DEV_SHOTS_START");
  const int start_ms = start_env ? int(std::atof(start_env) * 1000) : 0;
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  std::thread([runtime, &app_context, dir, interval_ms, count, start_ms] {
    std::this_thread::sleep_for(std::chrono::milliseconds(start_ms));
    for (int i = 1; i <= count; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
      app_context.CallInUIThread([runtime, dir, i] {
        rex::ui::RawImage img;
        auto* gfx = runtime->graphics_system();
        auto* presenter = gfx ? gfx->presenter() : nullptr;
        if (presenter && presenter->CaptureGuestOutput(img) && img.width) {
          SaveBmp(img, dir / ("shot" + std::to_string(i) + ".bmp"));
        }
      });
    }
  }).detach();
}

}  // namespace

void StartFind(uint8_t* membase, std::string text, int after_s) {
  std::thread([membase, text, after_s] {
    std::this_thread::sleep_for(std::chrono::seconds(after_s));
    int hits = 0;
    for (uint64_t a = 0; a < 0x100000000ull && hits < 40;) {
      MEMORY_BASIC_INFORMATION mbi{};
      if (!VirtualQuery(membase + a, &mbi, sizeof(mbi))) break;
      const uint64_t region_end = uint64_t(static_cast<uint8_t*>(mbi.BaseAddress) - membase) + mbi.RegionSize;
      if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
        const uint8_t* lo = membase + a;
        const uint8_t* hi = membase + region_end;
        for (const uint8_t* p = lo; hits < 40;) {
          p = std::search(p, hi, text.begin(), text.end());
          if (p == hi) break;
          const uint32_t at = uint32_t(p - membase);
          const uint8_t* from = std::max(lo, p - 0x8000);
          const uint8_t* to = std::min(hi, p + 0x8000);
          char name[32];
          std::snprintf(name, sizeof(name), "find_%08X.bin", at);
          std::ofstream(name, std::ios::binary).write(reinterpret_cast<const char*>(from), to - from);
          REXLOG_INFO("ABT find: '{}' at guest {:08X} (saved from {:08X})", text, at, uint32_t(from - membase));
          ++hits;
          p += text.size();
        }
      }
      a = region_end;
    }
    REXLOG_INFO("ABT find: {} hits", hits);
  }).detach();
}

void StartDevTools(rex::Runtime* runtime, rex::ui::WindowedAppContext& app_context, uint32_t image_base,
                   uint32_t image_size) {
  if (const char* dump_path = std::getenv("ABT_DUMP_IMAGE"); dump_path && *dump_path) {
    std::ofstream out(dump_path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(runtime->virtual_membase() + image_base), image_size);
    REXLOG_INFO("ABT: guest image written to {} ({} bytes)", dump_path, image_size);
  }
  if (const char* shots = std::getenv("ABT_DEV_SHOTS"); shots && *shots) StartShots(runtime, app_context, shots);
  if (const char* find = std::getenv("ABT_DEV_FIND"); find && *find) {
    const char* at = std::getenv("ABT_DEV_FIND_AT");
    StartFind(runtime->virtual_membase(), find, at ? std::atoi(at) : 30);
  }
  if (const char* audio = std::getenv("ABT_DEV_AUDIO"); audio && *audio == '1') {
    std::thread([membase = runtime->virtual_membase()] {
      for (int second = 1;; ++second) {
        float peak = 0.0f;
        for (int poll = 0; poll < 50; ++poll) {  // 50 looks a second, so a short sound is not missed
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          const uint8_t* p = membase + 0x8275C790u;
          for (int i = 0; i < 6 * 256; ++i, p += 4) {
            const uint32_t bits = uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
            float v;
            std::memcpy(&v, &bits, 4);
            if (v == v) peak = std::max(peak, v < 0 ? -v : v);
          }
        }
        REXLOG_INFO("ABT audio: second {} peak {:.3f}", second, peak);
      }
    }).detach();
  }
}

}  // namespace abt

// ABT_DEV_IO=1: logs every file the game opens through CreateFile (sub_82680908, path in r3) and the second XAPI
// open (sub_82680C88, path in r3), with the result.
namespace {
const bool g_dev_io = [] {
  const char* v = std::getenv("ABT_DEV_IO");
  return v && *v == '1';
}();

std::string GuestString(const uint8_t* base, uint32_t ptr) {
  std::string s;
  if (!ptr) return s;
  for (uint32_t i = 0; i < 260 && base[ptr + i]; ++i) s += char(base[ptr + i]);
  return s;
}
}  // namespace

#include <rex/hook.h>

REX_EXTERN(__imp__sub_82680908);
REX_HOOK_RAW(sub_82680908) {
  const uint32_t path = ctx.r3.u32;
  __imp__sub_82680908(ctx, base);
  if (g_dev_io) REXLOG_INFO("ABT io: CreateFile '{}' -> {:08X}", GuestString(base, path), ctx.r3.u32);
}

REX_EXTERN(__imp__sub_82680C88);
REX_HOOK_RAW(sub_82680C88) {
  const uint32_t path = ctx.r3.u32;
  __imp__sub_82680C88(ctx, base);
  if (g_dev_io) REXLOG_INFO("ABT io: open2 '{}' -> {:08X}", GuestString(base, path), ctx.r3.u32);
}

// ABT_DEV_IO=1 also logs the first call of functions that use the game's data files (found from the code that builds
// their names): the master stream (game:\stream.dat), the stream path, audio.pck and the Bink movie path.
#define ABT_FIRST_CALL(addr, what)                                                       \
  REX_EXTERN(__imp__sub_##addr);                                                         \
  REX_HOOK_RAW(sub_##addr) {                                                             \
    static std::atomic<bool> seen{false};                                                \
    if (g_dev_io && !seen.exchange(true)) REXLOG_INFO("ABT io: first call of {}", what); \
    __imp__sub_##addr(ctx, base);                                                        \
  }
ABT_FIRST_CALL(8255DDE8, "sub_8255DDE8 (master stream, game:\stream.dat)")
ABT_FIRST_CALL(821C8C18, "sub_821C8C18 (stream.dat path)")
ABT_FIRST_CALL(821D60F8, "sub_821D60F8 (audio.pck)")
ABT_FIRST_CALL(821DAAB0, "sub_821DAAB0 (movie path)")
// sub_821C4738(options, arg) reads one command line argument (-game:, -level:, -nosplash and others).
REX_EXTERN(__imp__sub_821C4738);
REX_HOOK_RAW(sub_821C4738) {
  const uint32_t arg = ctx.r4.u32;
  __imp__sub_821C4738(ctx, base);
  if (g_dev_io) REXLOG_INFO("ABT io: command line argument '{}' -> {}", GuestString(base, arg), ctx.r3.u32);
}
