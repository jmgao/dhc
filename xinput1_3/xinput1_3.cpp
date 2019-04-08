#include <windows.h>

// Hopefully, Microsoft maintained ABI compatibility between versions...
#include <xinput.h>

#include <cguid.h>

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

static uintptr_t device_count() {
  static uintptr_t cached = []() {
    dhc_init();
    return dhc_get_device_count();
  }();
  return cached;
}

#define CHECK_DEVICE_INDEX(idx)          \
  do {                                   \
    if (!dhc_xinput_is_enabled()) {      \
      return ERROR_DEVICE_NOT_CONNECTED; \
    } else if (idx >= device_count()) {  \
      return ERROR_DEVICE_NOT_CONNECTED; \
    }                                    \
  } while (0)

#define LOG_ONCE(msg)                                \
  static bool __attribute__((unused)) _once = []() { \
    LOG(WARNING) << msg;                             \
    return true;                                     \
  }()

DWORD WINAPI XInputGetState(DWORD user_index, XINPUT_STATE* state) {
  dhc_init();
  CHECK_DEVICE_INDEX(user_index);

  // This is a horrible, thread-unsafe hack, but it should be good enough.
  static DWORD packet_number;
  state->dwPacketNumber = packet_number++;

  dhc_update();
  DeviceInputs inputs = dhc_get_inputs(user_index);
  WORD buttons = 0;
  switch (inputs.hat_dpad) {
    case Hat::Neutral:
      break;
    case Hat::North:
      buttons |= XINPUT_GAMEPAD_DPAD_UP;
      break;
    case Hat::NorthEast:
      buttons |= XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_RIGHT;
      break;
    case Hat::East:
      buttons |= XINPUT_GAMEPAD_DPAD_RIGHT;
      break;
    case Hat::SouthEast:
      buttons |= XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_RIGHT;
      break;
    case Hat::South:
      buttons |= XINPUT_GAMEPAD_DPAD_DOWN;
      break;
    case Hat::SouthWest:
      buttons |= XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT;
      break;
    case Hat::West:
      buttons |= XINPUT_GAMEPAD_DPAD_LEFT;
      break;
    case Hat::NorthWest:
      buttons |= XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_LEFT;
      break;
  }

#define ASSIGN_BUTTON(value, field) \
  do {                              \
    if (inputs.field._0) {          \
      buttons |= value;             \
    }                               \
  } while (0)

  ASSIGN_BUTTON(XINPUT_GAMEPAD_START, button_start);
  ASSIGN_BUTTON(XINPUT_GAMEPAD_BACK, button_select);
  ASSIGN_BUTTON(XINPUT_GAMEPAD_LEFT_THUMB, button_l3);
  ASSIGN_BUTTON(XINPUT_GAMEPAD_RIGHT_THUMB, button_r3);
  ASSIGN_BUTTON(XINPUT_GAMEPAD_LEFT_SHOULDER, button_l1);
  ASSIGN_BUTTON(XINPUT_GAMEPAD_RIGHT_SHOULDER, button_r1);
  ASSIGN_BUTTON(XINPUT_GAMEPAD_A, button_south);
  ASSIGN_BUTTON(XINPUT_GAMEPAD_B, button_east);
  ASSIGN_BUTTON(XINPUT_GAMEPAD_X, button_west);
  ASSIGN_BUTTON(XINPUT_GAMEPAD_Y, button_north);

  state->Gamepad.wButtons = buttons;
  state->Gamepad.bLeftTrigger = inputs.axis_left_trigger._0 * 255;
  state->Gamepad.bRightTrigger = inputs.axis_right_trigger._0 * 255;

  auto clamp = [](double x) {
    if (x > 32767) {
      return 32767;
    } else if (x < -32768) {
      return -32768;
    } else {
      return static_cast<int>(x);
    }
  };

  state->Gamepad.sThumbLX = clamp(65536.0 * inputs.axis_left_stick_x._0 - 32768.0);
  state->Gamepad.sThumbLY = clamp(32768.0 - 65536.0 * inputs.axis_left_stick_y._0);
  state->Gamepad.sThumbRX = clamp(65536.0 * inputs.axis_right_stick_x._0 - 32768.0);
  state->Gamepad.sThumbRY = clamp(32768.0 - 65536.0 * inputs.axis_right_stick_y._0);
  return ERROR_SUCCESS;
}

DWORD WINAPI XInputSetState(DWORD user_index, XINPUT_VIBRATION* vibration) {
  dhc_init();
  CHECK_DEVICE_INDEX(user_index);
  LOG_ONCE("XInputSetState unimplemented");
  return ERROR_SUCCESS;
}

DWORD WINAPI XInputGetCapabilities(DWORD user_index, DWORD flags,
                                   XINPUT_CAPABILITIES* capabilities) {
  dhc_init();
  CHECK_DEVICE_INDEX(user_index);

  capabilities->Type = XINPUT_DEVTYPE_GAMEPAD;
  capabilities->SubType = XINPUT_DEVSUBTYPE_ARCADE_STICK;
  capabilities->Flags = 0;
  capabilities->Gamepad.wButtons = 0xFFFF;
  capabilities->Gamepad.bLeftTrigger = 0;
  capabilities->Gamepad.bRightTrigger = 0;
  capabilities->Gamepad.sThumbLX = 0;
  capabilities->Gamepad.sThumbLY = 0;
  capabilities->Gamepad.sThumbRX = 0;
  capabilities->Gamepad.sThumbRY = 0;
  capabilities->Vibration.wLeftMotorSpeed = 0;
  capabilities->Vibration.wRightMotorSpeed = 0;
  return ERROR_SUCCESS;
}

void WINAPI XInputEnable(BOOL enable) {
  dhc_init();
  if (!enable) {
    LOG_ONCE("XInputEnable unimplemented");
  }
}

DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD user_index, GUID* render_guid,
                                             GUID* capture_guid) {
  dhc_init();
  CHECK_DEVICE_INDEX(user_index);
  *render_guid = GUID_NULL;
  *capture_guid = GUID_NULL;
  return ERROR_SUCCESS;
}

DWORD WINAPI XInputGetBatteryInformation(DWORD user_index, BYTE dev_type,
                                         XINPUT_BATTERY_INFORMATION* battery_information) {
  dhc_init();
  CHECK_DEVICE_INDEX(user_index);
  battery_information->BatteryType = BATTERY_TYPE_WIRED;
  battery_information->BatteryLevel = BATTERY_LEVEL_FULL;
  return ERROR_SUCCESS;
}

DWORD WINAPI XInputGetKeystroke(DWORD user_index, DWORD reserved, XINPUT_KEYSTROKE* keystroke) {
  dhc_init();
  CHECK_DEVICE_INDEX(user_index);
  LOG_ONCE("XInputGetKeystroke is unimplemented");
  return ERROR_EMPTY;
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
