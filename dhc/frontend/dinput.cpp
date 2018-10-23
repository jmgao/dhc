#include <windows.h>

#define INITGUID
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "dhc/frontend/dinput.h"

#include <atomic>

#include "dhc/frontend/ps4.h"
#include "dhc/logging.h"
#include "dhc/utils.h"

using namespace dhc::logging;

namespace dhc {

static std::string to_string(REFGUID guid) {
  if (guid == GUID_SysKeyboard) {
    return "GUID_SysKeyboard";
  } else if (guid == GUID_SysMouse) {
    return "GUID_SysMouse";
  } else if (guid == GUID_DHC_P1) {
    return "GUID_DHC_P1";
  } else if (guid == GUID_DHC_P2) {
    return "GUID_DHC_P2";
  } else {
    return "unknown";
  }
}

template <typename CharType>
class EmulatedDirectInputDevice8;

template <typename CharType>
class EmulatedDirectInput8 : public com_base<DI8Interface<CharType>> {
 public:
  explicit EmulatedDirectInput8(com_ptr<DI8Interface<CharType>> real)
      : real_(std::move(real)),
        p1_(new EmulatedDirectInputDevice8<CharType>()),
        p2_(new EmulatedDirectInputDevice8<CharType>()) {}
  virtual ~EmulatedDirectInput8() = default;

  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** obj) override final {
    if (!obj) {
      return E_INVALIDARG;
    }

    const IID* expected;
    if (std::is_same<CharType, wchar_t>::value) {
      expected = &IID_IDirectInput8W;
    } else {
      expected = &IID_IDirectInput8A;
    }

    if (riid != *expected) {
      *obj = nullptr;
      return E_NOINTERFACE;
    }

    *obj = this;
    this->AddRef();
    return NOERROR;
  }

  virtual HRESULT STDMETHODCALLTYPE CreateDevice(REFGUID refguid,
                                                 DI8DeviceInterface<CharType>** device,
                                                 IUnknown* unknown) override final {
    if (refguid == GUID_SysKeyboard || refguid == GUID_SysMouse) {
      LOG(DEBUG) << "DirectInput8::CreateDevice(" << to_string(refguid) << ") = passthrough";
      return real_->CreateDevice(refguid, device, unknown);
    }

    if (refguid == GUID_DHC_P1 || refguid == GUID_DHC_P2) {
      bool is_p1 = refguid == GUID_DHC_P1;
      *device = (is_p1 ? p1_ : p2_).clone().release();
      return DI_OK;
    }

    LOG(DEBUG) << "DirectInput8::CreateDevice(" << to_string(refguid) << ") = DIERR_DEVICENOTREG";
    *device = nullptr;
    return DIERR_DEVICENOTREG;
  }

  using EnumDevicesCallback = BOOL(PASCAL*)(const DI8DeviceInstance<CharType>*, void*);
  virtual HRESULT STDMETHODCALLTYPE EnumDevices(DWORD dev_type, EnumDevicesCallback callback,
                                                void* callback_arg, DWORD flags) override final {
    LOG(DEBUG) << "DirectInput8::EnumDevices";

    // Assumed behavior.
    flags &= ~DIEDFL_ATTACHEDONLY;

    if (flags != 0) {
      LOG(FATAL) << "DirectInput8::EnumDevices received unhandled flags " << flags;
    }

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
      // TODO: Actually probe the real keyboard type?
      dev.dwDevType = DI8DEVTYPE_KEYBOARD | (DI8DEVTYPEKEYBOARD_PCENH << 8);
      tstrncpy(dev.tszInstanceName, "Keyboard", MAX_PATH);
      tstrncpy(dev.tszProductName, "Keyboard", MAX_PATH);
      if (callback(&dev, callback_arg) == DIENUM_STOP) {
        return DI_OK;
      }
    }

    if (enum_mouse) {
      DI8DeviceInstance<CharType> dev = {};
      dev.dwSize = sizeof(dev);
      dev.guidInstance = GUID_SysMouse;
      dev.guidProduct = GUID_SysMouse;
      // TODO: Actually probe the real mouse type?
      dev.dwDevType = DI8DEVTYPE_MOUSE | (DI8DEVTYPEMOUSE_TRADITIONAL << 8);
      tstrncpy(dev.tszInstanceName, "Mouse", MAX_PATH);
      tstrncpy(dev.tszProductName, "Mouse", MAX_PATH);
      if (callback(&dev, callback_arg) == DIENUM_STOP) {
        return DI_OK;
      }
    }

    if (enum_sticks) {
      for (auto& guid : {GUID_DHC_P1, GUID_DHC_P2}) {
        DI8DeviceInstance<CharType> dev = {};
        dev.dwSize = sizeof(dev);
        dev.guidInstance = guid;
        dev.guidProduct = guid;
        dev.dwDevType = DI8DEVTYPE_GAMEPAD | (DI8DEVTYPEGAMEPAD_STANDARD << 8);
        char player = guid.Data4[7];
        const char* name = (player == 1) ? "DHC P1" : "DHC P2";
        tstrncpy(dev.tszInstanceName, name, MAX_PATH);
        tstrncpy(dev.tszProductName, name, MAX_PATH);

        if (callback(&dev, callback_arg) == DIENUM_STOP) {
          return DI_OK;
        }
      }
    }

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

  virtual HRESULT STDMETHODCALLTYPE FindDevice(REFGUID guid, const CharType* name,
                                               GUID* instance) override final {
    LOG(DEBUG) << "DirectInput8::FindDevice(" << to_string(guid) << ", " << to_string(name) << ")";
    return DIERR_DEVICENOTREG;
  }

  using EnumDevicesBySemanticsCallback = BOOL(PASCAL*)(const DI8DeviceInstance<CharType>*,
                                                       DI8DeviceInterface<CharType>*, DWORD, DWORD,
                                                       void*);
  virtual HRESULT STDMETHODCALLTYPE EnumDevicesBySemantics(const CharType* username,
                                                           DI8ActionFormat<CharType>* action_format,
                                                           EnumDevicesBySemanticsCallback callback,
                                                           void* callback_arg,
                                                           DWORD flags) override final {
    LOG(FATAL) << "DirectInput8::EnumDevicesBySemantics unimplemented";
    return DI_OK;
  }

  using ConfigureDevicesCallback = BOOL(PASCAL*)(IUnknown*, LPVOID);
  virtual HRESULT STDMETHODCALLTYPE ConfigureDevices(ConfigureDevicesCallback callback,
                                                     DI8ConfigureDevicesParams<CharType>* params,
                                                     DWORD flags,
                                                     void* callback_data) override final {
    LOG(FATAL) << "DirectInput8::ConfigureDevices unimplemented";
    return DI_OK;
  }

 private:
  com_ptr<DI8Interface<CharType>> real_;
  com_ptr<EmulatedDirectInputDevice8<CharType>> p1_;
  com_ptr<EmulatedDirectInputDevice8<CharType>> p2_;
};

template <typename CharType>
class EmulatedDirectInputDevice8 : public com_base<DI8DeviceInterface<CharType>> {
 public:
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** obj) override final {
    if (!obj) {
      return E_INVALIDARG;
    }

    const IID* expected;
    if (std::is_same<CharType, wchar_t>::value) {
      expected = &IID_IDirectInputDevice8W;
    } else {
      expected = &IID_IDirectInputDevice8A;
    }

    if (riid != *expected) {
      *obj = nullptr;
      return E_NOINTERFACE;
    }

    *obj = this;
    this->AddRef();
    return NOERROR;
  }

  virtual HRESULT STDMETHODCALLTYPE GetCapabilities(DIDEVCAPS* caps) override final {
    LOG(VERBOSE) << "EmulatedDirectInputDevice8::GetCapabilities";
    caps->dwFlags = DIDC_ATTACHED | DIDC_EMULATED;
    caps->dwDevType = DI8DEVTYPE_GAMEPAD | (DI8DEVTYPEGAMEPAD_STANDARD << 8) | 0x10000 /* ??? */;

    // Pretend to be a PS4 controller.
    caps->dwAxes = 6;
    caps->dwButtons = 14;
    caps->dwPOVs = 1;

    caps->dwFFSamplePeriod = 0;
    caps->dwFFMinTimeResolution = 0;

    caps->dwFirmwareRevision = 0;
    caps->dwHardwareRevision = 0;
    caps->dwFFDriverVersion = 0;
    return DI_OK;
  }

  using EnumObjectsCallback = BOOL(PASCAL*)(const DI8DeviceObjectInstance<CharType>*, void*);
  virtual HRESULT STDMETHODCALLTYPE EnumObjects(EnumObjectsCallback callback, void* callback_arg,
                                                DWORD flags) override final {
    LOG(VERBOSE) << "EmulatedDirectInput8Device::EnumObjects(flags = " << flags << ")";
    std::vector<DirectInputButtonConf> objects;

    if (LOWORD(flags) >> 8) {
      // Asked for a non-zero enum collection.
      LOG(INFO) << "EmulatedDirectInput8Device::EnumObjects called with non-zero enum collection "
                << (LOWORD(flags) >> 8);
      return DI_OK;
    }

    if (flags & DIDFT_ABSAXIS) {
      objects.push_back(kDIAxisLeftX);
      objects.push_back(kDIAxisLeftY);

      objects.push_back(kDIAxisRightX);
      objects.push_back(kDIAxisRightY);

      objects.push_back(kDIAxisL2);
      objects.push_back(kDIAxisR2);
    }
    if (flags & DIDFT_POV) {
      objects.push_back(kDIDPad);
    }
    if (flags & DIDFT_PSHBUTTON) {
      objects.push_back(kDIButtonSquare);
      objects.push_back(kDIButtonCross);
      objects.push_back(kDIButtonCircle);
      objects.push_back(kDIButtonTriangle);

      objects.push_back(kDIButtonL1);
      objects.push_back(kDIButtonR1);

      objects.push_back(kDIButtonL2);
      objects.push_back(kDIButtonR2);

      objects.push_back(kDIButtonShare);
      objects.push_back(kDIButtonOptions);

      objects.push_back(kDIButtonL3);
      objects.push_back(kDIButtonR3);

      objects.push_back(kDIButtonPS);
      objects.push_back(kDIButtonTrackpad);
    }
    if (flags & DIDFT_COLLECTION) {
      objects.push_back(kDICollectionGamepad);
    }

    for (const auto& object : objects) {
      DI8DeviceObjectInstance<CharType> obj = {};
      obj.dwSize = sizeof(obj);
      obj.guidType = object.guid;
      obj.dwOfs = object.offset;
      obj.dwType = object.type;
      obj.dwFlags = object.flags;
      tstrncpy(obj.tszName, object.name, MAX_PATH);
      // TODO: HID stuff?
      if (callback(&obj, callback_arg) != DIENUM_CONTINUE) {
        return DI_OK;
      }
    }

    return DI_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetProperty(REFGUID, DIPROPHEADER*) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE SetProperty(REFGUID guid,
                                                const DIPROPHEADER* prop_header) override final {
    std::string property_name;

    // These aren't equivalent to `guid == DIPROP_FOO`, because fuck you, that's why.
    if (&guid == &DIPROP_APPDATA) {
      property_name = "DIPROP_APPDATA";
    } else if (&guid == &DIPROP_AUTOCENTER) {
      property_name = "DIPROP_AUTOCENTER";
    } else if (&guid == &DIPROP_AXISMODE) {
      property_name = "DIPROP_AXISMODE";
    } else if (&guid == &DIPROP_BUFFERSIZE) {
      property_name = "DIPROP_BUFFERSIZE";
    } else if (&guid == &DIPROP_CALIBRATION) {
      property_name = "DIPROP_CALIBRATION";
    } else if (&guid == &DIPROP_CALIBRATIONMODE) {
      property_name = "DIPROP_CALIBRATIONMODE";
    } else if (&guid == &DIPROP_CPOINTS) {
      property_name = "DIPROP_CPOINTS";
    } else if (&guid == &DIPROP_DEADZONE) {
      property_name = "DIPROP_DEADZONE";
    } else if (&guid == &DIPROP_FFGAIN) {
      property_name = "DIPROP_FFGAIN";
    } else if (&guid == &DIPROP_INSTANCENAME) {
      property_name = "DIPROP_INSTANCENAME";
    } else if (&guid == &DIPROP_PRODUCTNAME) {
      property_name = "DIPROP_PRODUCTNAME";
    } else if (&guid == &DIPROP_RANGE) {
      property_name = "DIPROP_RANGE";
    } else if (&guid == &DIPROP_SATURATION) {
      property_name = "DIPROP_SATURATION";
    }

    LOG(VERBOSE) << "EmulatedDirectInput8Device::SetProperty(" << property_name << ")";
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE Acquire() override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE Unacquire() override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE GetDeviceState(DWORD, void*) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE GetDeviceData(DWORD, DIDEVICEOBJECTDATA*, DWORD*,
                                                  DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE SetDataFormat(const DIDATAFORMAT* format) override final {
    LOG(VERBOSE) << "EmulatedDirectInput8Device::SetDataFormat";

    if (sizeof(DIDATAFORMAT) != format->dwSize) {
      LOG(ERROR) << "EmulatedDirectInput8Device::SetDataFormat: received invalid dwSize "
                 << format->dwSize << " (expected " << sizeof(DIDATAFORMAT) << ")";
      return DIERR_INVALIDPARAM;
    }

    if (sizeof(DIOBJECTDATAFORMAT) != format->dwObjSize) {
      LOG(ERROR) << "EmulatedDirectInput8Device::SetDataFormat: received invalid dwObjSize "
                 << format->dwObjSize << " (expected " << sizeof(DIOBJECTDATAFORMAT) << ")";
      return DIERR_INVALIDPARAM;
    }

    if (format->dwNumObjs <= 0) {
      LOG(ERROR) << "EmulatedDirectInput8Device::SetDataFormat: received invalid dwNumObjs "
                 << format->dwNumObjs;
      return DIERR_INVALIDPARAM;
    }

    // TODO: Validate the object data format?
    object_data_format_.assign(format->rgodf, format->rgodf + format->dwNumObjs);
    return DI_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE SetEventNotification(HANDLE) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND, DWORD flags) override final {
    std::vector<std::string> stringified_flags;
    if (flags != 0) {
      if (flags & DISCL_BACKGROUND) {
        stringified_flags.push_back("DISCL_BACKGROUND");
      }
      if (flags & DISCL_EXCLUSIVE) {
        stringified_flags.push_back("DISCL_EXCLUSIVE");
      }
      if (flags & DISCL_FOREGROUND) {
        stringified_flags.push_back("DISCL_FOREGROUND");
      }
      if (flags & DISCL_NONEXCLUSIVE) {
        stringified_flags.push_back("DISCL_NONEXCLUSIVE");
      }
      if (flags & DISCL_NOWINKEY) {
        stringified_flags.push_back("DISCL_NOWINKEY");
      }
    }

    LOG(VERBOSE) << "EmulatedDirectInput8Device::SetCooperativeLevel("
                 << ((flags == 0) ? "0" : Join(stringified_flags, " | ")) << ")";

    return DI_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetObjectInfo(DI8DeviceObjectInstance<CharType>*, DWORD,
                                                  DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE GetDeviceInfo(DI8DeviceInstance<CharType>*) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE RunControlPanel(HWND, DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE Initialize(HINSTANCE, DWORD, REFGUID) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE CreateEffect(REFGUID, const DIEFFECT*, IDirectInputEffect**,
                                                 IUnknown*) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  using EnumEffectsCallback = BOOL(PASCAL*)(const DI8EffectInfo<CharType>*, void*);
  virtual HRESULT STDMETHODCALLTYPE EnumEffects(EnumEffectsCallback, void*, DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE GetEffectInfo(DI8EffectInfo<CharType>*,
                                                  REFGUID) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE GetForceFeedbackState(DWORD*) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE SendForceFeedbackCommand(DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  using EnumCreatedEffectObjectsCallback = BOOL(PASCAL*)(IDirectInputEffect*, void*);
  virtual HRESULT STDMETHODCALLTYPE EnumCreatedEffectObjects(EnumCreatedEffectObjectsCallback,
                                                             void*, DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE Escape(DIEFFESCAPE*) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE Poll() override final {
    LOG(VERBOSE) << "EmulatedDirectInput8Device::Poll()";
    return DI_NOEFFECT;
  }

  virtual HRESULT STDMETHODCALLTYPE SendDeviceData(DWORD, const DIDEVICEOBJECTDATA*, DWORD*,
                                                   DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  using EnumEffectsInFileCallback = BOOL(PASCAL*)(const DIFILEEFFECT*, void*);
  virtual HRESULT STDMETHODCALLTYPE EnumEffectsInFile(const CharType*, EnumEffectsInFileCallback,
                                                      void*, DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE WriteEffectToFile(const CharType*, DWORD, DIFILEEFFECT*,
                                                      DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE BuildActionMap(DI8ActionFormat<CharType>*, const CharType*,
                                                   DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE SetActionMap(DI8ActionFormat<CharType>*, const CharType*,
                                                 DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE
  GetImageInfo(DI8DeviceImageInfoHeader<CharType>*) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

 private:
  std::vector<DIOBJECTDATAFORMAT> object_data_format_;
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
