#include <windows.h>

// Hopefully, Microsoft maintained ABI compatibility between versions...
#include <xinput.h>

#include "dhc/dhc.h"
#include "dhc/logging.h"

extern "C" {

BOOL WINAPI DllMain(HMODULE module, DWORD reason, void*) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(module);
      break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}

static bool init() {
  static bool initialized = []() {
    dhc_init();
    return true;
  }();
  return initialized;
}

DWORD WINAPI XInputGetState(DWORD user_index, XINPUT_STATE* state) {
  init();
  return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputSetState(DWORD user_index, XINPUT_VIBRATION* vibration) {
  init();
  return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetCapabilities(DWORD user_index, DWORD flags,
                                   XINPUT_CAPABILITIES* capabilities) {
  init();
  return ERROR_DEVICE_NOT_CONNECTED;
}

void WINAPI XInputEnable(BOOL enable) {
  init();
}

DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD user_index, GUID* render_guid,
                                             GUID* capture_guid) {
  init();
  return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetBatteryInformation(DWORD user_index, BYTE dev_type,
                                         XINPUT_BATTERY_INFORMATION* battery_information) {
  init();
  return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetKeystroke(DWORD user_index, DWORD reserved, XINPUT_KEYSTROKE* keystroke) {
  init();
  return ERROR_DEVICE_NOT_CONNECTED;
}

void WINAPI Unknown100() {
  UNIMPLEMENTED(FATAL);
}

void WINAPI Unknown101() {
  UNIMPLEMENTED(FATAL);
}

void WINAPI Unknown102() {
  UNIMPLEMENTED(FATAL);
}

void WINAPI Unknown103() {
  UNIMPLEMENTED(FATAL);
}

}  //  extern "C"
