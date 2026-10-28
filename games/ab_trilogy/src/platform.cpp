// Small operating system helpers.

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace abt {

// True when the window in front belongs to this process.
bool OwnsForegroundWindow() {
#if defined(_WIN32)
  DWORD pid = 0;
  const HWND foreground = GetForegroundWindow();
  return foreground && GetWindowThreadProcessId(foreground, &pid) && pid == GetCurrentProcessId();
#else
  return false;
#endif
}

}  // namespace abt
