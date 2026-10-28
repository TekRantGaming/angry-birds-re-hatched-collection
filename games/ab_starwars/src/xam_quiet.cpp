// XamXStudioRequest(code, request): the game's Microsoft Studios telemetry (an event code and a small record with a
// timestamp, e.g. sub_825F3900 and sub_82611808 build one and return straight away; no caller reads the result).
// The runtime's stub logs a warning on every call (about 75 a second from the game's own threads) and leaves r3 as it
// was. This version answers success quietly, which is what a console that queues the event would return. Defining it
// here takes the place of the runtime DLL's stub for the game's direct calls.

#include <atomic>

#include <rex/hook.h>
#include <rex/logging.h>

extern "C" REX_FUNC(__imp__XamXStudioRequest) {
  (void)base;
  static std::atomic<bool> logged{false};
  if (!logged.exchange(true)) REXLOG_INFO("ABSW: XamXStudioRequest answered quietly (telemetry, not sent)");
  ctx.r3.u64 = 0;  // ERROR_SUCCESS
}
