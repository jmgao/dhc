#pragma once

#include <dinput.h>

#include "dhc.h"

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
  using ActionFormatType = DIACTIONFORMATA;
  using ConfigureDevicesParamsType = DICONFIGUREDEVICESPARAMSA;
};

template <>
struct DI8Types<wchar_t> {
  using CharType = wchar_t;
  using InterfaceType = IDirectInput8W;
  using DeviceInterfaceType = IDirectInputDevice8W;
  using DeviceInstanceType = DIDEVICEINSTANCEW;
  using ActionFormatType = DIACTIONFORMATW;
  using ConfigureDevicesParamsType = DICONFIGUREDEVICESPARAMSW;
};

template <typename CharType>
using DI8Interface = typename DI8Types<CharType>::InterfaceType;

template <typename CharType>
using DI8DeviceInterface = typename DI8Types<CharType>::DeviceInterfaceType;

template <typename CharType>
using DI8DeviceInstance = typename DI8Types<CharType>::DeviceInstanceType;

template <typename CharType>
using DI8ActionFormat = typename DI8Types<CharType>::ActionFormatType;

template <typename CharType>
using DI8ConfigureDevicesParams = typename DI8Types<CharType>::ConfigureDevicesParamsType;

}  // namespace dhc
