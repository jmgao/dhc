#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Hopefully, Microsoft maintained ABI compatibility between versions...
#include <Xinput.h>

#include "logging.h"
#include "utils.h"
#include "xinput.h"

// This declaration is hidden when targeting Windows 8 or newer.
DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD user_index, GUID* render_guid, GUID* capture_guid);

static FARPROC WINAPI GetXInputProc(const char* proc_name) {
  static HMODULE real = dhc::LoadSystemLibrary(L"xinput1_3.dll");
  FARPROC result = GetProcAddress(real, proc_name);
  if (!result) {
    LOG(FATAL) << "failed to resolve symbol '" << proc_name << "'";
  }
  return result;
}

struct PassthroughXInput : public dhc::XInputImplementation {
  virtual DWORD GetState(DWORD user_index, XINPUT_STATE* state) override final {
    static auto real = reinterpret_cast<decltype(&XInputGetState)>(GetXInputProc("XInputGetState"));
    DWORD rc = real(user_index, state);
    LOG(VERBOSE) << "XInputGetState(" << user_index << ") = " << rc;
    return rc;
  }

  virtual DWORD SetState(DWORD user_index, XINPUT_VIBRATION* vibration) override final {
    static auto real = reinterpret_cast<decltype(&XInputSetState)>(GetXInputProc("XInputSetState"));
    DWORD rc = real(user_index, vibration);
    LOG(VERBOSE) << "XInputGetState(" << user_index << ") = " << rc;
    return rc;
  }

  virtual DWORD GetCapabilities(DWORD user_index, DWORD flags, XINPUT_CAPABILITIES* capabilities) override final {
    static auto real = reinterpret_cast<decltype(&XInputGetCapabilities)>(GetXInputProc("XInputGetCapabilities"));
    DWORD rc = real(user_index, flags, capabilities);
    LOG(VERBOSE) << "XInputGetCapabilities(" << user_index << ") = " << rc;
    return rc;
  }

  virtual void Enable(BOOL value) override final {
    static auto real = reinterpret_cast<decltype(&XInputEnable)>(GetXInputProc("XInputEnable"));
    LOG(VERBOSE) << "XInputEnable(" << value << ")";
    real(value);
  }

  virtual DWORD GetDSoundAudioDeviceGuids(DWORD user_index, GUID* render_guid, GUID* capture_guid) override final {
    static auto real = reinterpret_cast<decltype(&XInputGetDSoundAudioDeviceGuids)>(GetXInputProc("XInputGetDSoundAudioDeviceGuids"));
    LOG(VERBOSE) << "XInputGetDSoundAudioDeviceGuids(" << user_index << ")";
    return real(user_index, render_guid, capture_guid);
  }

  virtual DWORD GetBatteryInformation(DWORD user_index, BYTE dev_type, XINPUT_BATTERY_INFORMATION* battery_information) override final {
    static auto real = reinterpret_cast<decltype(&XInputGetBatteryInformation)>(GetXInputProc("XInputGetBatteryInformation"));
    LOG(VERBOSE) << "XInputGetBatteryInformation(" << user_index << ")";
    return real(user_index, dev_type, battery_information);
  }

  virtual DWORD GetKeystroke(DWORD user_index, DWORD reserved, XINPUT_KEYSTROKE* keystroke) override final {
    static auto real = reinterpret_cast<decltype(&XInputGetKeystroke)>(GetXInputProc("XInputGetKeystroke"));
    LOG(VERBOSE) << "XInputGetKeystroke(" << user_index << ")";
    return real(user_index, reserved, keystroke);
  }

  static PassthroughXInput& Instance() {
    static PassthroughXInput instance;
    return instance;
  }
};

struct EmptyXInput : public dhc::XInputImplementation {
  virtual DWORD GetState(DWORD user_index, XINPUT_STATE* state) override final {
    return ERROR_DEVICE_NOT_CONNECTED;
  }

  virtual DWORD SetState(DWORD user_index, XINPUT_VIBRATION* vibration) override final {
    return ERROR_DEVICE_NOT_CONNECTED;
  }

  virtual DWORD GetCapabilities(DWORD user_index, DWORD flags, XINPUT_CAPABILITIES* capabilities) override final {
    return ERROR_DEVICE_NOT_CONNECTED;
  }

  virtual void Enable(BOOL value) override final {
    LOG(ERROR) << "unhandled XInputEnable(" << value << ")";
    return;
  }

  virtual DWORD GetDSoundAudioDeviceGuids(DWORD user_index, GUID* render_guid, GUID* capture_guid) override final {
    return ERROR_DEVICE_NOT_CONNECTED;
  }

  virtual DWORD GetBatteryInformation(DWORD user_index, BYTE dev_type, XINPUT_BATTERY_INFORMATION* battery_information) override final {
    return ERROR_DEVICE_NOT_CONNECTED;
  }

  virtual DWORD GetKeystroke(DWORD user_index, DWORD reserved, XINPUT_KEYSTROKE* keystroke) override final {
    return ERROR_DEVICE_NOT_CONNECTED;
  }

  static EmptyXInput& Instance() {
    static EmptyXInput instance;
    return instance;
  }
};

dhc::XInputImplementation& dhc::XInputImplementation::Instance() {
#if 1
  return EmptyXInput::Instance();
#else
  return PassthroughXInput::Instance();
#endif
}