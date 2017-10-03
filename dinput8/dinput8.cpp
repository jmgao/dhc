#define INITGUID
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dhc/dhc.h"
#include "dhc/dinput.h"
#include "dhc/logging.h"
#include "dhc/utils.h"

using namespace dhc::logging;

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

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD version, REFIID desired_interface, void** out_interface, IUnknown* unknown) {
  bool unicode = desired_interface == IID_IDirectInput8W;
  if (!unicode) {
    CHECK(IID_IDirectInput8A == desired_interface);
  }

  LOG(INFO) << "requested DirectInput8 " << (unicode ? "unicode" : "ascii") << " interface, with" << (unknown ? "" : "out") << " COM interface";

#if 0
  // Passthrough
  static auto real = reinterpret_cast<decltype(&DirectInput8Create)>(GetDirectInput8Proc("DirectInput8Create"));
  return real(hinst, version, desired_interface, out_interface, unknown);
#else
  IUnknown* result;
  if (unicode) {
    result = dhc::GetEmulatedDirectInput8W();
  } else {
    result = dhc::GetEmulatedDirectInput8W();
  }

  result->AddRef();
  *out_interface = result;
  return DI_OK;
#endif
}

extern "C" HRESULT WINAPI DllCanUnloadNow() {
  static auto real = reinterpret_cast<decltype(&DllCanUnloadNow)>(dhc::GetDirectInput8Proc("DllCanUnloadNow"));
  return real();
}

extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
  static auto real = reinterpret_cast<decltype(&DllGetClassObject)>(dhc::GetDirectInput8Proc("DllGetClassObject"));
  return real(rclsid, riid, ppv);
}

extern "C" HRESULT WINAPI DllRegisterServer() {
  static auto real = reinterpret_cast<decltype(&DllRegisterServer)>(dhc::GetDirectInput8Proc("DllRegisterServer"));
  return real();
}

extern "C" HRESULT WINAPI DllUnregisterServer() {
  static auto real = reinterpret_cast<decltype(&DllUnregisterServer)>(dhc::GetDirectInput8Proc("DllUnregisterServer"));
  return real();
}

extern "C" void WINAPI GetdfDIJoystick() {
  UNIMPLEMENTED(FATAL);
}
