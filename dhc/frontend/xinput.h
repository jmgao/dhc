#pragma once

#include "dhc/dhc.h"

#include <xinput.h>

namespace dhc {

struct XInputImplementation {
  DHC_API virtual DWORD GetState(DWORD, XINPUT_STATE*) = 0;
  DHC_API virtual DWORD SetState(DWORD, XINPUT_VIBRATION*) = 0;
  DHC_API virtual DWORD GetCapabilities(DWORD, DWORD, XINPUT_CAPABILITIES*) = 0;
  DHC_API virtual void Enable(BOOL) = 0;
  DHC_API virtual DWORD GetDSoundAudioDeviceGuids(DWORD, GUID*, GUID*) = 0;
  DHC_API virtual DWORD GetBatteryInformation(DWORD, BYTE, XINPUT_BATTERY_INFORMATION*) = 0;
  DHC_API virtual DWORD GetKeystroke(DWORD, DWORD, XINPUT_KEYSTROKE*) = 0;

  static DHC_API XInputImplementation& Instance();
};

}  // namespace dhc
