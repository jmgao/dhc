#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define INITGUID
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <atomic>

#include "dinput.h"
#include "logging.h"
#include "utils.h"

using namespace dhc::logging;

namespace dhc {

static std::string to_string(REFGUID guid) {
  if (guid == GUID_SysKeyboard) {
    return "GUID_SysKeyboard";
  } else if (guid == GUID_SysMouse) {
    return "GUID_SysMouse";
  } else {
    return "unknown";
  }
}

FARPROC WINAPI GetDirectInput8Proc(const char* proc_name) {
  static HMODULE real = dhc::LoadSystemLibrary(L"dinput8.dll");
  return GetProcAddress(real, proc_name);
}

static HRESULT RealDirectInput8Create(HINSTANCE hinst, DWORD version, REFIID desired_interface, void** out_interface, IUnknown* unknown) {
  static auto real = reinterpret_cast<decltype(&DirectInput8Create)>(GetDirectInput8Proc("DirectInput8Create"));
  return real(hinst, version, desired_interface, out_interface, unknown);
}

com_ptr<IDirectInput8W> GetRealDirectInput8W() {
  void* iface;
  HRESULT rc = RealDirectInput8Create(HINST_SELF, 0x0800, IID_IDirectInput8W, &iface, nullptr);
  CHECK_EQ(DI_OK, rc);
  return com_ptr<IDirectInput8W>(static_cast<IDirectInput8W*>(iface));
}

com_ptr<IDirectInput8A> GetRealDirectInput8A() {
  void* iface;
  HRESULT rc = RealDirectInput8Create(HINST_SELF, 0x0800, IID_IDirectInput8A, &iface, nullptr);
  CHECK_EQ(DI_OK, rc);
  return com_ptr<IDirectInput8A>(static_cast<IDirectInput8A*>(iface));
}

template <typename CharType>
class EmulatedDirectInput8 : public DI8Interface<CharType> {
 public:
  explicit EmulatedDirectInput8(com_ptr<DI8Interface<CharType>> real) : real_(std::move(real)) {}
  virtual ~EmulatedDirectInput8() = default;

  COM_OBJECT_BASE();

  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** obj) override final {
    if (!obj) {
      return E_INVALIDARG;
    }

    const IID* expected;
    if (std::is_same_v<CharType, wchar_t>) {
      expected = &IID_IDirectInput8W;
    } else {
      expected = &IID_IDirectInput8A;
    }

    if (riid != *expected) {
      *obj = nullptr;
      return E_NOINTERFACE;
    }

    *obj = this;
    AddRef();
    return NOERROR;
  }

  virtual HRESULT STDMETHODCALLTYPE CreateDevice(REFGUID refguid, DI8DeviceInterface<CharType>** device,
                                                 IUnknown* unknown) override final {
    if (refguid == GUID_SysKeyboard || refguid == GUID_SysMouse) {
      LOG(DEBUG) << "DirectInput8::CreateDevice(" << to_string(refguid) << ") = passthrough";
      return real_->CreateDevice(refguid, device, unknown);
    }

    LOG(DEBUG) << "DirectInput8::CreateDevice(" << to_string(refguid) << ") = DIERR_DEVICENOTREG";
    *device = nullptr;
    return DIERR_DEVICENOTREG;
  }

  using EnumDevicesCallback = BOOL(PASCAL*)(const DI8DeviceInstance<CharType>*, void*);
  virtual HRESULT STDMETHODCALLTYPE EnumDevices(DWORD dev_type, EnumDevicesCallback callback, void* callback_arg,
                                                DWORD flags) override final {
    LOG(DEBUG) << "DirectInput8::EnumDevices";
    bool enum_keyboard = dev_type == DI8DEVCLASS_KEYBOARD;
    bool enum_mouse = dev_type == DI8DEVCLASS_POINTER;
    bool enum_sticks = dev_type == DI8DEVCLASS_GAMECTRL;

    if (dev_type == DI8DEVCLASS_ALL) {
      enum_keyboard = enum_mouse = enum_sticks = true;
    }

    if (enum_keyboard) {
      DI8DeviceInstance<CharType> dev = {};
      dev.dwSize = sizeof(dev);
      dev.guidInstance = GUID_SysKeyboard;
      dev.guidProduct = GUID_SysKeyboard;
      dev.dwDevType = DI8DEVTYPE_KEYBOARD | DI8DEVTYPEKEYBOARD_PCENH;  // TODO: Actually probe the real keyboard type?
      tstrncpy(dev.tszInstanceName, "Keyboard", MAX_PATH);
      tstrncpy(dev.tszProductName, "Keyboard", MAX_PATH);
      callback(&dev, callback_arg);
    }

    if (enum_mouse) {
      DI8DeviceInstance<CharType> dev = {};
      dev.dwSize = sizeof(dev);
      dev.guidInstance = GUID_SysMouse;
      dev.guidProduct = GUID_SysMouse;
      dev.dwDevType = DI8DEVTYPE_MOUSE | DI8DEVTYPEMOUSE_TRADITIONAL;  // TODO: Actually probe the real mouse type?
      tstrncpy(dev.tszInstanceName, "Mouse", MAX_PATH);
      tstrncpy(dev.tszProductName, "Mouse", MAX_PATH);
      callback(&dev, callback_arg);
    }

    // TODO: Present our fake devices, once they exist.
    return DI_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetDeviceStatus(REFGUID refguid) override final {
    LOG(DEBUG) << "DirectInput8::GetDeviceStatus(" << to_string(refguid) << ")";
    return DI_NOTATTACHED;
  }

  virtual HRESULT STDMETHODCALLTYPE RunControlPanel(HWND owner, DWORD flags) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_INVALIDPARAM;
  }

  virtual HRESULT STDMETHODCALLTYPE Initialize(HINSTANCE hinst, DWORD version) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_INVALIDPARAM;
  }

  virtual HRESULT STDMETHODCALLTYPE FindDevice(REFGUID refguid, const CharType* name, GUID* instance) override final {
    LOG(DEBUG) << "DirectInput8::FindDevice(" << to_string(refguid) << ", " << to_string(name) << ")";
    return DIERR_DEVICENOTREG;
  }

  using EnumDevicesBySemanticsCallback = BOOL(PASCAL*)(const DI8DeviceInstance<CharType>*,
                                                       DI8DeviceInterface<CharType>*, DWORD, DWORD, void*);
  virtual HRESULT STDMETHODCALLTYPE EnumDevicesBySemantics(const CharType* username,
                                                           DI8ActionFormat<CharType>* action_format,
                                                           EnumDevicesBySemanticsCallback callback, void* callback_arg,
                                                           DWORD flags) override final {
    LOG(DEBUG) << "DirectInput8::EnumDevicesBySemantics";
    return DI_OK;
  }

  using ConfigureDevicesCallback = BOOL(PASCAL*)(IUnknown*, LPVOID);
  virtual HRESULT STDMETHODCALLTYPE ConfigureDevices(ConfigureDevicesCallback callback,
                                                     DI8ConfigureDevicesParams<CharType>* params, DWORD flags,
                                                     void* callback_data) override final {
    LOG(DEBUG) << "DirectInput8::ConfigureDevices";
    return DI_OK;
  }

 private:
  unique_com_ptr<InterfaceType> real_;
};

using EmulatedDirectInput8W = EmulatedDirectInput8<wchar_t>;
using EmulatedDirectInput8A = EmulatedDirectInput8<char>;

IDirectInput8W* GetEmulatedDirectInput8W() {
  static auto instance = new EmulatedDirectInput8W(GetRealDirectInput8W());
  return instance;
}

IDirectInput8A* GetEmulatedDirectInput8A() {
  static auto instance = new EmulatedDirectInput8A(GetRealDirectInput8A());
  return instance;
}

}  // namespace dhc
