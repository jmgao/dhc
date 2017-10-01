#define INITGUID
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dhc/dhc.h"
#include "dhc/logging.h"

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

static FARPROC WINAPI GetDirectInputProc(const char* proc_name) {
  static HMODULE real = LoadSystemLibrary(L"dinput8.dll");
  return GetProcAddress(real, proc_name);
}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD version, REFIID desired_interface, void** out_interface, IUnknown* unknown) {
  bool unicode = desired_interface == IID_IDirectInput8W;
  if (!unicode) {
    CHECK(IID_IDirectInput8A == desired_interface);
  }

  LOG(INFO) << "requested DirectInput8 " << (unicode ? "unicode" : "ascii") << " interface, with" << (unknown ? "" : "out") << " COM interface";
  static auto real = reinterpret_cast<decltype(&DirectInput8Create)>(GetDirectInputProc("DirectInput8Create"));
  return real(hinst, version, desired_interface, out_interface, unknown);
}

extern "C" HRESULT WINAPI DllCanUnloadNow() {
  static auto real = reinterpret_cast<decltype(&DllCanUnloadNow)>(GetDirectInputProc("DllCanUnloadNow"));
  return real();
}

extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
  static auto real = reinterpret_cast<decltype(&DllGetClassObject)>(GetDirectInputProc("DllGetClassObject"));
  return real(rclsid, riid, ppv);
}

extern "C" HRESULT WINAPI DllRegisterServer() {
  static auto real = reinterpret_cast<decltype(&DllRegisterServer)>(GetDirectInputProc("DllRegisterServer"));
  return real();
}

extern "C" HRESULT WINAPI DllUnregisterServer() {
  static auto real = reinterpret_cast<decltype(&DllUnregisterServer)>(GetDirectInputProc("DllUnregisterServer"));
  return real();
}

extern "C" void WINAPI GetdfDIJoystick() {
  UNIMPLEMENTED(FATAL);
}