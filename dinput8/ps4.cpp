#include <dinput.h>

#include <vector>

#include "dhc_dinput.h"

namespace dhc {

std::vector<EmulatedDeviceObject> GeneratePS4EmulatedDeviceObjects() {
  std::vector<EmulatedDeviceObject> result;
  // Axes
  result.push_back({.name = "X Axis",
                    .guid = GUID_XAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 0,
                    .offset = 12,
                    .mapped_object = AxisType::LeftStickX});
  result.push_back({.name = "Y Axis",
                    .guid = GUID_YAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 1,
                    .offset = 8,
                    .mapped_object = AxisType::LeftStickY});
  result.push_back({.name = "Z Axis",
                    .guid = GUID_ZAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 2,
                    .offset = 4,
                    .mapped_object = AxisType::RightStickX});
  result.push_back({.name = "Z Rotation",
                    .guid = GUID_RzAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 5,
                    .offset = 0,
                    .mapped_object = AxisType::RightStickY});

#if defined(ENABLE_TRIGGERS)
  result.push_back({.name = "X Rotation",
                    .guid = GUID_RxAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 3,
                    .offset = 28,
                    .mapped_object = AxisType::LeftTrigger});
  result.push_back({.name = "Y Rotation",
                    .guid = GUID_RyAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 4,
                    .offset = 24,
                    .mapped_object = AxisType::RightTrigger});
#endif

  // Buttons
  result.push_back({.name = "Button 0",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 0,
                    .offset = 220,
                    .mapped_object = ButtonType::West});
  result.push_back({.name = "Button 1",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 1,
                    .offset = 221,
                    .mapped_object = ButtonType::South});
  result.push_back({.name = "Button 2",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 2,
                    .offset = 222,
                    .mapped_object = ButtonType::East});
  result.push_back({.name = "Button 3",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 3,
                    .offset = 223,
                    .mapped_object = ButtonType::North});
  result.push_back({.name = "Button 4",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 4,
                    .offset = 224,
                    .mapped_object = ButtonType::L1});
  result.push_back({.name = "Button 5",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 5,
                    .offset = 225,
                    .mapped_object = ButtonType::R1});
  result.push_back({.name = "Button 6",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 6,
                    .offset = 226,
                    .mapped_object = ButtonType::L2});
  result.push_back({.name = "Button 7",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 7,
                    .offset = 227,
                    .mapped_object = ButtonType::R2});
  result.push_back({.name = "Button 8",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 8,
                    .offset = 228,
                    .mapped_object = ButtonType::Select});
  result.push_back({.name = "Button 9",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 9,
                    .offset = 229,
                    .mapped_object = ButtonType::Start});
  result.push_back({.name = "Button 10",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 10,
                    .offset = 230,
                    .mapped_object = ButtonType::L3});
  result.push_back({.name = "Button 11",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 11,
                    .offset = 231,
                    .mapped_object = ButtonType::R3});
  result.push_back({.name = "Button 12",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 12,
                    .offset = 232,
                    .mapped_object = ButtonType::Home});
  result.push_back({.name = "Button 13",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 13,
                    .offset = 233,
                    .mapped_object = ButtonType::Trackpad});

  // D-Pad
  result.push_back({.name = "Hat Switch",
                    .guid = GUID_POV,
                    .type = DIDFT_POV,
                    .flags = 0,
                    .instance_id = 0,
                    .offset = 16,
                    .mapped_object = HatType::DPad});

  return result;
}

}  // namespace dhc
