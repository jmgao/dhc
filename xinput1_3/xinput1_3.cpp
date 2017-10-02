#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Hopefully, Microsoft maintained ABI compatibility between versions...
#include <Xinput.h>

#include "dhc/dhc.h"
#include "dhc/logging.h"
#include "dhc/xinput.h"

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

DWORD WINAPI XInputGetState(DWORD user_index, XINPUT_STATE* state) {
  return dhc::XInputImplementation::Instance().GetState(user_index, state);
}

DWORD WINAPI XInputSetState(DWORD user_index, XINPUT_VIBRATION* vibration) {
  return dhc::XInputImplementation::Instance().SetState(user_index, vibration);
}

DWORD WINAPI XInputGetCapabilities(DWORD user_index, DWORD flags, XINPUT_CAPABILITIES* capabilities) {
  return dhc::XInputImplementation::Instance().GetCapabilities(user_index, flags, capabilities);
}

void WINAPI XInputEnable(BOOL enable) {
  return dhc::XInputImplementation::Instance().Enable(enable);
}

DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD user_index, GUID* render_guid, GUID* capture_guid) {
  return dhc::XInputImplementation::Instance().GetDSoundAudioDeviceGuids(user_index, render_guid, capture_guid);
}

DWORD WINAPI XInputGetBatteryInformation(DWORD user_index, BYTE dev_type, XINPUT_BATTERY_INFORMATION* battery_information) {
  return dhc::XInputImplementation::Instance().GetBatteryInformation(user_index, dev_type, battery_information);
}

DWORD WINAPI XInputGetKeystroke(DWORD user_index, DWORD reserved, XINPUT_KEYSTROKE* keystroke) {
  return dhc::XInputImplementation::Instance().GetKeystroke(user_index, reserved, keystroke);
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