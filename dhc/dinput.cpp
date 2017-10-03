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

unique_com_ptr<IDirectInput8W> GetRealDirectInput8W() {
  void* iface;
  HRESULT rc = RealDirectInput8Create(HINST_SELF, 0x0800, IID_IDirectInput8W, &iface, nullptr);
  CHECK_EQ(DI_OK, rc);
  return unique_com_ptr<IDirectInput8W>(static_cast<IDirectInput8W*>(iface));
}

unique_com_ptr<IDirectInput8A> GetRealDirectInput8A() {
  void* iface;
  HRESULT rc = RealDirectInput8Create(HINST_SELF, 0x0800, IID_IDirectInput8A, &iface, nullptr);
  CHECK_EQ(DI_OK, rc);
  return unique_com_ptr<IDirectInput8A>(static_cast<IDirectInput8A*>(iface));
}

template <typename CharType, typename InterfaceType, typename DeviceInterfaceType, typename DeviceInstanceType,
          typename ActionFormatType, typename ConfigureParamsType>
class EmulatedDirectInput8 : public InterfaceType {
 public:
  explicit EmulatedDirectInput8(unique_com_ptr<InterfaceType> real) : real_(std::move(real)) {}
  virtual ~EmulatedDirectInput8() = default;

  COM_OBJECT_BASE();

  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** obj) override final {
    if (!obj) {
      return E_INVALIDARG;
    }

    const IID* expected;
    if (std::is_same_v<InterfaceType, IDirectInput8W>) {
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

  virtual HRESULT STDMETHODCALLTYPE CreateDevice(REFGUID refguid, DeviceInterfaceType** device,
                                                 IUnknown* unknown) override final {
    if (refguid == GUID_SysKeyboard || refguid == GUID_SysMouse) {
      LOG(DEBUG) << "DirectInput8::CreateDevice(" << to_string(refguid) << ") = passthrough";
      return real_->CreateDevice(refguid, device, unknown);
    }

    LOG(DEBUG) << "DirectInput8::CreateDevice(" << to_string(refguid) << ") = DIERR_DEVICENOTREG";
    return DIERR_DEVICENOTREG;
  }

  using EnumDevicesCallback = BOOL(FAR PASCAL*)(const DeviceInstanceType*, void*);
  virtual HRESULT STDMETHODCALLTYPE EnumDevices(DWORD dev_type, EnumDevicesCallback callback, void* callback_arg,
                                                DWORD flags) override final {
    LOG(DEBUG) << "DirectInput8::EnumDevices";
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

  using EnumDevicesBySemanticsCallback = BOOL(FAR PASCAL*)(const DeviceInstanceType*, DeviceInterfaceType*, DWORD,
                                                           DWORD, void*);
  virtual HRESULT STDMETHODCALLTYPE EnumDevicesBySemantics(const CharType* username, ActionFormatType* action_format,
                                                           EnumDevicesBySemanticsCallback callback, void* callback_arg,
                                                           DWORD flags) override final {
    LOG(DEBUG) << "DirectInput8::EnumDevicesBySemantics";
    return DI_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK callback, ConfigureParamsType* params,
                                                     DWORD flags, void* callback_data) override final {
    LOG(DEBUG) << "DirectInput8::ConfigureDevices";
    return DI_OK;
  }

 private:
  unique_com_ptr<InterfaceType> real_;
};

using EmulatedDirectInput8W = EmulatedDirectInput8<wchar_t, IDirectInput8W, IDirectInputDevice8W, DIDEVICEINSTANCEW,
                                                   DIACTIONFORMATW, DICONFIGUREDEVICESPARAMSW>;
using EmulatedDirectInput8A = EmulatedDirectInput8<char, IDirectInput8A, IDirectInputDevice8A, DIDEVICEINSTANCEA,
                                                   DIACTIONFORMATA, DICONFIGUREDEVICESPARAMSA>;

IDirectInput8W* GetEmulatedDirectInput8W() {
  static auto instance = new EmulatedDirectInput8W(GetRealDirectInput8W());
  return instance;
}

IDirectInput8A* GetEmulatedDirectInput8A() {
  static auto instance = new EmulatedDirectInput8A(GetRealDirectInput8A());
  return instance;
}

}  // namespace dhc
