#pragma once

#include <dinput.h>

#include "dhc/dhc.h"

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

struct DirectInputButtonConf {
  DWORD flags;
  DWORD offset;
  DWORD type;
  GUID guid;
  const char* name;
};

static const DirectInputButtonConf kDIAxisLeftX = {
    DIDOI_ASPECTPOSITION,
    12,
    DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE(0),
    GUID_XAxis,
    "X Axis",
};

static const DirectInputButtonConf kDIAxisLeftY = {
    DIDOI_ASPECTPOSITION,
    8,
    DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE(1),
    GUID_YAxis,
    "Y Axis",
};

static const DirectInputButtonConf kDIAxisRightX = {
    DIDOI_ASPECTPOSITION,
    4,
    DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE(2),
    GUID_ZAxis,
    "Z Axis",
};

static const DirectInputButtonConf kDIAxisRightY = {
    DIDOI_ASPECTPOSITION,
    0,
    DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE(5),
    GUID_RzAxis,
    "Z Rotation",
};

static const DirectInputButtonConf kDIAxisL2 = {
    DIDOI_ASPECTPOSITION,
    28,
    DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE(3),
    GUID_RxAxis,
    "X Rotation",
};

static const DirectInputButtonConf kDIAxisR2 = {
    DIDOI_ASPECTPOSITION,
    24,
    DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE(4),
    GUID_RxAxis,
    "Y Rotation",
};

static const DirectInputButtonConf kDIDPad = {
    0,
    16,
    DIDFT_POV | DIDFT_MAKEINSTANCE(0),
    GUID_POV,
    "Hat Switch",
};

// No idea what this is, but it's what consistently gets returned for a PS4 controller.
static constexpr GUID GUID_Button = {
    0xa36d02f0,
    0xc9f3,
    0x11cf,
    {0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00},
};

static const DirectInputButtonConf kDIButtonSquare = {
    0,
    220,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(0),
    GUID_Button,
    "Button 0",
};

static const DirectInputButtonConf kDIButtonX = {
    0,
    221,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(1),
    GUID_Button,
    "Button 1",
};

static const DirectInputButtonConf kDIButtonCircle = {
    0,
    222,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(2),
    GUID_Button,
    "Button 2",
};
static const DirectInputButtonConf kDIButtonTriangle = {
    0,
    223,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(3),
    GUID_Button,
    "Button 3",
};

static const DirectInputButtonConf kDIButtonL1 = {
    0,
    224,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(4),
    GUID_Button,
    "Button 4",
};
static const DirectInputButtonConf kDIButtonR1 = {
    0,
    225,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(5),
    GUID_Button,
    "Button 5",
};

static const DirectInputButtonConf kDIButtonL2 = {
    0,
    226,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(6),
    GUID_Button,
    "Button 6",
};

static const DirectInputButtonConf kDIButtonR2 = {
    0,
    227,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(7),
    GUID_Button,
    "Button 7",
};

static const DirectInputButtonConf kDIButtonShare = {
    0,
    228,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(8),
    GUID_Button,
    "Button 8",
};

static const DirectInputButtonConf kDIButtonOptions = {
    0,
    229,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(9),
    GUID_Button,
    "Button 9",
};

static const DirectInputButtonConf kDIButtonL3 = {
    0,
    230,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(10),
    GUID_Button,
    "Button 10",
};

static const DirectInputButtonConf kDIButtonR3 = {
    0,
    231,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(11),
    GUID_Button,
    "Button 11",
};

static const DirectInputButtonConf kDIButtonPS = {
    0,
    232,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(12),
    GUID_Button,
    "Button 12",
};

static const DirectInputButtonConf kDIButtonTrackpad = {
    0,
    233,
    DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE(1),
    GUID_Button,
    "Button 2",
};

static const DirectInputButtonConf kDICollectionGamepad = {
    0,
    0,
    DIDFT_COLLECTION | DIDFT_NODATA,
    GUID_Unknown,
    "Collection 0 - Game Pad",
};
}  // namespace dhc
