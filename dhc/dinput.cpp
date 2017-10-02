#define INITGUID
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

template <typename CharType, typename InterfaceType, typename DeviceInterfaceType, typename DeviceInstanceType,
          typename ActionFormatType, typename ConfigureParamsType>
class EmulatedDirectInput8 : public InterfaceType {
 public:
  EmulatedDirectInput8() : ref_count_(1) {}

  virtual ~EmulatedDirectInput8() = default;

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

  virtual ULONG STDMETHODCALLTYPE AddRef() override final { return ++ref_count_; }

  virtual ULONG STDMETHODCALLTYPE Release() override final {
    ULONG rc = --ref_count_;
    if (rc == 0) {
      delete this;
    }
    return rc;
  }

  virtual HRESULT STDMETHODCALLTYPE CreateDevice(REFGUID refguid, DeviceInterfaceType** device,
                                                 IUnknown* unknown) override final {
    LOG(DEBUG) << "DirectInput8::CreateDevice(" << to_string(refguid) << ")";
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

  std::atomic<ULONG> ref_count_;
};

using EmulatedDirectInput8W = EmulatedDirectInput8<wchar_t, IDirectInput8W, IDirectInputDevice8W, DIDEVICEINSTANCEW,
                                                   DIACTIONFORMATW, DICONFIGUREDEVICESPARAMSW>;
using EmulatedDirectInput8A = EmulatedDirectInput8<char, IDirectInput8A, IDirectInputDevice8A, DIDEVICEINSTANCEA,
                                                   DIACTIONFORMATA, DICONFIGUREDEVICESPARAMSA>;

IDirectInput8W* GetEmulatedDirectInput8W() {
  static auto instance = new EmulatedDirectInput8W();
  return instance;
}

IDirectInput8A* GetEmulatedDirectInput8A() {
  static auto instance = new EmulatedDirectInput8A();
  return instance;
}

}  // namespace dhc
