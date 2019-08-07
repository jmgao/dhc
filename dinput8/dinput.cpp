#include <windows.h>

#define INITGUID
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <deque>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "dhc/dhc.h"
#include "dhc/logging.h"
#include "dhc_dinput.h"

using namespace std::string_literals;

namespace dhc {

template <typename CharType>
class EmulatedDirectInputDevice8;

// Helpers for GetProperty/SetProperty.
#define DEVICE_PROPERTY(name)                                                                 \
  do {                                                                                        \
    if (prop_header->dwHow != DIPH_DEVICE) {                                                  \
      LOG(WARNING) << "SetProperty(" << #name << ") called with invalid dwHow"; \
      return DIERR_INVALIDPARAM;                                                              \
    }                                                                                         \
  } while (0)

#define UNIMPLEMENTED_DEVICE_PROPERTY(name)         \
  do {                                              \
    DEVICE_PROPERTY(name);                          \
    UNIMPLEMENTED(FATAL) << #name " unimplemented"; \
  } while (0)


// TODO: If a process uses both ASCII and Unicode interfaces, should they share state?
template <typename CharType>
class EmulatedDirectInput8 : public com_base<DI8Interface<CharType>> {
 public:
  explicit EmulatedDirectInput8(com_ptr<DI8Interface<CharType>> real) : real_(std::move(real)) {
    p1_.reset(new EmulatedDirectInputDevice8<CharType>(0));
    p2_.reset(new EmulatedDirectInputDevice8<CharType>(1));
  }

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
    LOG(FATAL) << "DirectInput8::GetDeviceStatus(" << to_string(refguid) << ")";
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
    LOG(FATAL) << "DirectInput8::FindDevice(" << to_string(guid) << ", " << to_string(name) << ")";
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
  explicit EmulatedDirectInputDevice8(uintptr_t vdev_idx) : vdev_(vdev_idx) {
    objects_ = GeneratePS4EmulatedDeviceObjects();
  }

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
    LOG(VERBOSE) << "EmulatedDirectInput8Device::EnumObjects(" << didft_to_string(flags) << ")";

    if (LOWORD(flags) >> 8) {
      // Asked for a non-zero enum collection.
      LOG(INFO) << "EmulatedDirectInput8Device::EnumObjects called with non-zero enum collection "
                << (LOWORD(flags) >> 8);
      return DI_OK;
    }

    for (const auto& object : objects_) {
      if (!object.MatchesFlags(flags)) {
        continue;
      }

      DI8DeviceObjectInstance<CharType> obj = {};
      obj.dwSize = sizeof(obj);
      obj.guidType = object.guid;
      obj.dwOfs = object.offset;
      obj.dwType = object.type | DIDFT_MAKEINSTANCE(object.instance_id);
      obj.dwFlags = object.flags;
      tstrncpy(obj.tszName, object.name, MAX_PATH);

      LOG(VERBOSE) << "Enumerating object " << obj.tszName << ": " << didft_to_string(obj.dwType);

      if (callback(&obj, callback_arg) != DIENUM_CONTINUE) {
        return DI_OK;
      }
    }

    return DI_OK;
  }

  std::string_view GetDIPropName(REFGUID guid) {
    if (&guid == &DIPROP_APPDATA) {
      return "DIPROP_APPDATA";
    } else if (&guid == &DIPROP_AUTOCENTER) {
      return "DIPROP_AUTOCENTER";
    } else if (&guid == &DIPROP_AXISMODE) {
      return "DIPROP_AXISMODE";
    } else if (&guid == &DIPROP_BUFFERSIZE) {
      return "DIPROP_BUFFERSIZE";
    } else if (&guid == &DIPROP_CALIBRATION) {
      return "DIPROP_CALIBRATION";
    } else if (&guid == &DIPROP_CALIBRATIONMODE) {
      return "DIPROP_CALIBRATIONMODE";
    } else if (&guid == &DIPROP_CPOINTS) {
      return "DIPROP_CPOINTS";
    } else if (&guid == &DIPROP_DEADZONE) {
      return "DIPROP_DEADZONE";
    } else if (&guid == &DIPROP_FFGAIN) {
      return "DIPROP_FFGAIN";
    } else if (&guid == &DIPROP_INSTANCENAME) {
      return "DIPROP_INSTANCENAME";
    } else if (&guid == &DIPROP_PRODUCTNAME) {
      return "DIPROP_PRODUCTNAME";
    } else if (&guid == &DIPROP_RANGE) {
      return "DIPROP_RANGE";
    } else if (&guid == &DIPROP_SATURATION) {
      return "DIPROP_SATURATION";
    }
    return "<unknown>";
  }

  virtual HRESULT STDMETHODCALLTYPE GetProperty(REFGUID, DIPROPHEADER*) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  bool FindPropertyObject(observer_ptr<EmulatedDeviceObject>* out_object,
                          const DIPROPHEADER* prop_header) {
    switch (prop_header->dwHow) {
      case DIPH_DEVICE:
        LOG(WARNING) << "FindPropertyObject(DIPH_DEVICE)";
        return false;

      case DIPH_BYOFFSET:
        LOG(DEBUG) << "FindPropertyObject(DIPH_BYOFFSET(" << prop_header->dwObj << ")";
        for (const auto& format : device_formats_) {
          if (format.offset == prop_header->dwObj) {
            *out_object = format.object;
            return true;
          }
        }
        return false;

      case DIPH_BYUSAGE:
        LOG(FATAL) << "DIPH_BYUSAGE unimplemented";
        return false;

      case DIPH_BYID:
        LOG(DEBUG) << "FindPropertyObject(DIPH_BYID(" << didft_to_string(prop_header->dwObj)
                   << "))";
        for (auto& object : objects_) {
          if (object.MatchesType(prop_header->dwObj)) {
            *out_object = observer_ptr<EmulatedDeviceObject>(&object);
            return true;
          }
        }
        return false;

      default:
        LOG(FATAL) << "invalid DIPROPHEADER::dwHow: " << prop_header->dwHow;
    }

    __builtin_unreachable();
  }

  virtual HRESULT STDMETHODCALLTYPE SetProperty(REFGUID guid,
                                                const DIPROPHEADER* prop_header) override final {
    if (prop_header->dwHeaderSize != sizeof(DIPROPHEADER)) {
      LOG(ERROR) << "SetProperty got invalid header size: " << prop_header->dwHeaderSize;
      return DIERR_INVALIDPARAM;
    }

    // Several properties are device-wide:
    //    DIPROP_AUTOCENTER, DIPROP_AXISMODE, DIPROP_BUFFERSIZE, DIPROP_FFGAIN,
    //    DIPROP_INSTANCENAME, DIPROP_PRODUCTNAME
    if (&guid == &DIPROP_AUTOCENTER) {
      UNIMPLEMENTED_DEVICE_PROPERTY(DIPROP_AUTOCENTER);
    } else if (&guid == &DIPROP_AXISMODE) {
      LOG(WARNING) << "DIPROP_AXISMODE unimplemented";
      return DI_OK;
    } else if (&guid == &DIPROP_BUFFERSIZE) {
      UNIMPLEMENTED_DEVICE_PROPERTY(DIPROP_BUFFERSIZE);
    } else if (&guid == &DIPROP_FFGAIN) {
      UNIMPLEMENTED_DEVICE_PROPERTY(DIPROP_FFGAIN);
    } else if (&guid == &DIPROP_INSTANCENAME) {
      UNIMPLEMENTED_DEVICE_PROPERTY(DIPROP_INSTANCENAME);
    } else if (&guid == &DIPROP_PRODUCTNAME) {
      UNIMPLEMENTED_DEVICE_PROPERTY(DIPROP_PRODUCTNAME);
    }

    // Find the object that's referenced.
    observer_ptr<EmulatedDeviceObject> object;
    if (!FindPropertyObject(&object, prop_header)) {
      LOG(ERROR) << "EmulatedDirectInput8Device::SetProperty(" << GetDIPropName(guid)
                 << ") failed to find object";
      return DIERR_OBJECTNOTFOUND;
    }

    LOG(DEBUG) << "EmulatedDirectInput8Device::SetProperty(" << GetDIPropName(guid) << ", "
               << object->name << ")";

    // These aren't equivalent to `guid == DIPROP_FOO`, because fuck you, that's why.
    if (&guid == &DIPROP_DEADZONE || &guid == &DIPROP_SATURATION) {
      if (prop_header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
      if (!(object->type & DIDFT_AXIS)) return DIERR_INVALIDPARAM;
      DWORD value = reinterpret_cast<const DIPROPDWORD*>(prop_header)->dwData;
      if (value < 0 || value > 10000) {
        // TODO: Does the reference implementation return an error here?
        return DIERR_INVALIDPARAM;
      }
      if (&guid == &DIPROP_DEADZONE) {
        LOG(DEBUG) << "Setting dead zone for axis " << object->name << " to " << value;
        object->deadzone = value / 10000.0;
      } else if (&guid == &DIPROP_SATURATION) {
        LOG(DEBUG) << "Setting saturation for axis " << object->name << " to " << value;
        object->saturation = value / 10000.0;
      }
      return DI_OK;
    } else if (&guid == &DIPROP_RANGE) {
      if (!(object->type & DIDFT_AXIS)) {
        LOG(DEBUG) << "attempted to set DIPROP_RANGE on non-axis";
        return DIERR_INVALIDPARAM;
      }

      if (prop_header->dwSize != sizeof(DIPROPRANGE)) {
        LOG(ERROR) << "dwSize mismatch";
        return DIERR_INVALIDPARAM;
      }

      const DIPROPRANGE* range = reinterpret_cast<const DIPROPRANGE*>(prop_header);
      // TODO: Should we check that max > min?
      LOG(DEBUG) << "Setting range for axis " << object->name << " to [" << range->lMin << ", "
                 << range->lMax << "]";
      std::tie(object->range_min, object->range_max) = std::tie(range->lMin, range->lMax);
      return DI_OK;
    }

    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE Acquire() override final {
    LOG(WARNING) << "Acquire unimplemented";
    return DI_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE Unacquire() override final {
    LOG(WARNING) << "Unacquire unimplemented";
    return DI_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetDeviceState(DWORD size, void* buffer) override final {
    LOG(VERBOSE) << "EmulatedDirectInput8Device::GetDeviceState(" << size << ")";
    memset(buffer, 0, size);
    DeviceInputs inputs = dhc_get_inputs(vdev_);
    for (const auto& fmt : device_formats_) {
      fmt.Apply(static_cast<char*>(buffer), size, inputs);
    }
    for (const auto& fmt_default : device_format_defaults_) {
      *reinterpret_cast<DWORD*>(static_cast<char*>(buffer) + fmt_default.offset) =
          fmt_default.value;
    }
    return DI_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetDeviceData(DWORD, DIDEVICEOBJECTDATA*, DWORD*,
                                                  DWORD) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE SetDataFormat(const DIDATAFORMAT* data_format) override final {
    LOG(VERBOSE) << "EmulatedDirectInput8Device::SetDataFormat";

    if (sizeof(DIDATAFORMAT) != data_format->dwSize) {
      LOG(ERROR) << "EmulatedDirectInput8Device::SetDataFormat: received invalid dwSize "
                 << data_format->dwSize << " (expected " << sizeof(DIDATAFORMAT) << ")";
      return DIERR_INVALIDPARAM;
    }

    if (sizeof(DIOBJECTDATAFORMAT) != data_format->dwObjSize) {
      LOG(ERROR) << "EmulatedDirectInput8Device::SetDataFormat: received invalid dwObjSize "
                 << data_format->dwObjSize << " (expected " << sizeof(DIOBJECTDATAFORMAT) << ")";
      return DIERR_INVALIDPARAM;
    }

    if (data_format->dwNumObjs <= 0) {
      LOG(ERROR) << "EmulatedDirectInput8Device::SetDataFormat: received invalid dwNumObjs "
                 << data_format->dwNumObjs;
      return DIERR_INVALIDPARAM;
    }

    for (size_t i = 0; i < data_format->dwNumObjs; ++i) {
      DIOBJECTDATAFORMAT* object_data_format = &data_format->rgodf[i];
      LOG(VERBOSE) << "DIObjectDataFormat " << i;
      if (object_data_format->pguid) {
        LOG(VERBOSE) << " GUID = " << to_string(*object_data_format->pguid);
      } else {
        LOG(VERBOSE) << " GUID = <none>";
      }
      LOG(VERBOSE) << "  offset = " << object_data_format->dwOfs;
      LOG(VERBOSE) << "  type = " << didft_to_string(object_data_format->dwType);
      LOG(VERBOSE) << "  flags = " << didoi_to_string(object_data_format->dwFlags);

      bool matched = false;
      for (auto& object : objects_) {
        if (object.matched) {
          continue;
        }

        if (!object.MatchesType(object_data_format->dwType)) {
          continue;
        }

        if (!object.MatchesFlags(object_data_format->dwFlags)) {
          continue;
        }

        if (object_data_format->pguid && *object_data_format->pguid != object.guid) {
          continue;
        }
        LOG(VERBOSE) << "  matched object format to " << object.name;
        matched = true;
        object.matched = true;
        device_formats_.push_back({.object = observer_ptr<EmulatedDeviceObject>(&object),
                                   .offset = object_data_format->dwOfs});
        break;
      }

      if (!matched) {
        if ((object_data_format->dwType & DIDFT_OPTIONAL)) {
          if (object_data_format->pguid && *object_data_format->pguid == GUID_POV) {
            device_format_defaults_.push_back({.offset = object_data_format->dwOfs, .value = -1UL});
          }
          LOG(VERBOSE) << "failed to match optional object";
        } else {
          LOG(ERROR) << "failed to match required object";
          return DIERR_OBJECTNOTFOUND;
        }
      }
    }
    LOG(VERBOSE) << "SetDataFormat done";
    return DI_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE SetEventNotification(HANDLE) override final {
    UNIMPLEMENTED(FATAL);
    return DIERR_NOTINITIALIZED;
  }

  virtual HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND, DWORD flags) override final {
    // TODO: Does this implicitly Acquire?
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
    dhc_update();
    return DI_OK;
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
  uintptr_t vdev_;
  std::vector<DIOBJECTDATAFORMAT> object_data_format_;
  std::vector<EmulatedDeviceObject> objects_;
  std::vector<DeviceFormat> device_formats_;
  std::vector<DeviceFormatDefault> device_format_defaults_;
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

void DeviceFormat::Apply(char* output_buffer, size_t output_buffer_length,
                         DeviceInputs inputs) const {
  std::visit(
      [&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          if (object->type & DIDFT_BUTTON) {
            output_buffer[offset] = 0;
            CHECK_GE(output_buffer_length, offset + 1);
          } else if (object->type & DIDFT_AXIS) {
            CHECK_EQ(0ULL, offset % 4);
            CHECK_GE(output_buffer_length, offset + 4);
            *reinterpret_cast<DWORD*>(&output_buffer[offset]) =
                (object->range_min + object->range_max) / 2;
          } else {
            LOG(FATAL) << "unhandled type " << object->type;
          }
        } else if constexpr (std::is_same_v<T, AxisType>) {
          CHECK(object->type & DIDFT_AXIS);
          CHECK_EQ(0ULL, offset % 4);
          CHECK_GE(output_buffer_length, offset + 4);
          auto axis = dhc_get_axis(inputs, arg);
          double value = axis._0;
          double distance = abs(value - 0.5);
          if (distance * 2 >= object->saturation) {
            value = value > 0.5 ? 1.0 : 0.0;
          } else if (distance * 2 <= object->deadzone) {
            value = 0.5;
          }

          DWORD lerped = static_cast<DWORD>(lerp(value, object->range_min, object->range_max));
          LOG(VERBOSE) << "lerping " << object->name << " value " << value << " onto ["
                       << object->range_min << ", " << object->range_max
                       << "] = " << static_cast<long>(lerped);
          *reinterpret_cast<DWORD*>(&output_buffer[offset]) = lerped;
        } else if constexpr (std::is_same_v<T, ButtonType>) {
          CHECK(object->type & DIDFT_BUTTON);
          CHECK_GE(output_buffer_length, offset + 1);
          auto value = dhc_get_button(inputs, arg)._0;
          output_buffer[offset] = value ? -128 : 0;
        } else if constexpr (std::is_same_v<T, HatType>) {
          CHECK(object->type & DIDFT_POV);
          CHECK_EQ(0ULL, offset % 4);
          CHECK_GE(output_buffer_length, offset + 4);
          auto hat = dhc_get_hat(inputs, arg);
          DWORD value = 0;
          switch (hat) {
          case Hat::Neutral:
            value = -1;
            break;

          case Hat::North:
            value = 0;
            break;

          case Hat::NorthEast:
            value = 4500;
            break;

          case Hat::East:
            value = 9000;
            break;

          case Hat::SouthEast:
            value = 13500;
            break;

          case Hat::South:
            value = 18000;
            break;

          case Hat::SouthWest:
            value = 22500;
            break;

          case Hat::West:
            value = 27000;
            break;

          case Hat::NorthWest:
            value = 31500;
            break;
          }
          *reinterpret_cast<DWORD *>(&output_buffer[offset]) = value;
        } else {
          LOG(FATAL) << "unhandled type?";
        }
      },
      object->mapped_object);
}

}  // namespace dhc

BOOL WINAPI DllMain(HMODULE module, DWORD reason, void *) {
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

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD version,
                                             REFIID desired_interface,
                                             void **out_interface,
                                             IUnknown *unknown) {
  dhc_init();

  bool unicode = desired_interface == IID_IDirectInput8W;
  if (!unicode) {
    CHECK(IID_IDirectInput8A == desired_interface);
  }

  LOG(INFO) << "requested DirectInput8 " << (unicode ? "unicode" : "ascii")
            << " interface, with" << (unknown ? "" : "out") << " COM interface";

  IUnknown *result;
  if (unicode) {
    result = dhc::GetEmulatedDirectInput8W();
  } else {
    result = dhc::GetEmulatedDirectInput8A();
  }

  result->AddRef();
  *out_interface = result;
  return DI_OK;
}

extern "C" HRESULT WINAPI DllCanUnloadNow() { return S_FALSE; }

extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid,
                                            void **ppv) {
  UNIMPLEMENTED(FATAL);
  abort();
}

extern "C" HRESULT WINAPI DllRegisterServer() {
  UNIMPLEMENTED(FATAL);
  abort();
}

extern "C" HRESULT WINAPI DllUnregisterServer() {
  UNIMPLEMENTED(FATAL);
  abort();
}

extern "C" void WINAPI GetdfDIJoystick() {
  UNIMPLEMENTED(FATAL);
  abort();
}
