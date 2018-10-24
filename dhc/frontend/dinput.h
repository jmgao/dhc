#pragma once

#include <dinput.h>

#include <variant>

#include "dhc/dhc.h"
#include "dhc/logging.h"
#include "dhc/utils.h"

namespace dhc {

DHC_API FARPROC WINAPI GetDirectInput8Proc(const char* proc_name);

DHC_API IDirectInput8W* GetEmulatedDirectInput8W();
DHC_API IDirectInput8A* GetEmulatedDirectInput8A();

template <typename T>
struct DI8Types;

template <>
struct DI8Types<char> {
  using CharType = char;
  using InterfaceType = IDirectInput8A;
  using DeviceInterfaceType = IDirectInputDevice8A;
  using DeviceInstanceType = DIDEVICEINSTANCEA;
  using DeviceObjectInstanceType = DIDEVICEOBJECTINSTANCEA;
  using ActionFormatType = DIACTIONFORMATA;
  using ConfigureDevicesParamsType = DICONFIGUREDEVICESPARAMSA;
  using EffectInfoType = DIEFFECTINFOA;
  using DeviceImageInfoHeaderType = DIDEVICEIMAGEINFOHEADERA;
};

template <>
struct DI8Types<wchar_t> {
  using CharType = wchar_t;
  using InterfaceType = IDirectInput8W;
  using DeviceInterfaceType = IDirectInputDevice8W;
  using DeviceInstanceType = DIDEVICEINSTANCEW;
  using DeviceObjectInstanceType = DIDEVICEOBJECTINSTANCEW;
  using ActionFormatType = DIACTIONFORMATW;
  using ConfigureDevicesParamsType = DICONFIGUREDEVICESPARAMSW;
  using EffectInfoType = DIEFFECTINFOW;
  using DeviceImageInfoHeaderType = DIDEVICEIMAGEINFOHEADERW;
};

template <typename CharType>
using DI8Interface = typename DI8Types<CharType>::InterfaceType;

template <typename CharType>
using DI8DeviceInterface = typename DI8Types<CharType>::DeviceInterfaceType;

template <typename CharType>
using DI8DeviceInstance = typename DI8Types<CharType>::DeviceInstanceType;

template <typename CharType>
using DI8DeviceObjectInstance = typename DI8Types<CharType>::DeviceObjectInstanceType;

template <typename CharType>
using DI8ActionFormat = typename DI8Types<CharType>::ActionFormatType;

template <typename CharType>
using DI8ConfigureDevicesParams = typename DI8Types<CharType>::ConfigureDevicesParamsType;

template <typename CharType>
using DI8EffectInfo = typename DI8Types<CharType>::EffectInfoType;

template <typename CharType>
using DI8DeviceImageInfoHeader = typename DI8Types<CharType>::DeviceImageInfoHeaderType;

// A struct representing an input for a given device.
struct EmulatedDeviceObject {
  const char* name;

  // GUID for the object type.
  GUID guid;

  // DIDFT_ABSAXIS, RELAXIS, PSHBUTTON, TGLBUTTON, POV, etc.
  // Note that several of the constants are bitmasks; the values here should be individual types.
  DWORD type;

  // DIDEVICEOBJECTINSTANCE::dwFlags
  // Should probably always contain DIDOI_POLLED?
  DWORD flags;

  // DIDFT_MAKEINSTANCE(instance_id)
  size_t instance_id;

  // Native data format of the object.
  // TODO: Does this matter?
  size_t offset;

  // Backend object that this object maps to, or std::monostate if it's unmapped.
  std::variant<std::monostate, AxisType, ButtonType, PovType> mapped_object;

  // Properties set by SetProperty:
  long range_min = 0;
  long range_max = 65535;

  double deadzone = 0.0;
  double saturation = 1.0;

  // Consumed by a DIOBJECTDATAFORMAT yet?
  bool matched = false;

  bool MatchesType(DWORD didft) {
    if (didft == DIDFT_ALL) {
      return true;
    }

    DWORD type_mask = DIDFT_GETTYPE(didft);
    if ((type_mask & type) == 0) {
      return false;
    }
    didft &= ~type_mask;

    DWORD instance_mask = didft & DIDFT_INSTANCEMASK;
    if (instance_mask != DIDFT_ANYINSTANCE && DIDFT_GETINSTANCE(instance_mask) != instance_id) {
      return false;
    }
    didft &= ~instance_mask;

    didft &= ~DIDFT_OPTIONAL;

    if (didft != 0) {
      LOG(INFO) << "leftover flags: " << didft_to_string(didft);
      return false;
    }

    return true;
  }

  bool MatchesFlags(DWORD didoi) const {
    return (didoi & flags) == didoi;
  }

  DWORD Identifier() const {
    return type | DIDFT_MAKEINSTANCE(instance_id);
  }
};

std::vector<EmulatedDeviceObject> GeneratePS4EmulatedDeviceObjects();

struct DeviceFormat {
  observer_ptr<EmulatedDeviceObject> object;
  size_t offset;

  void Apply(char* output_buffer, size_t output_buffer_length,
             observer_ptr<Device> virtual_device) const;
};


}  // namespace dhc
