#include <dinput.h>

#include <vector>

#include "dhc/frontend/dinput.h"

namespace dhc {

std::vector<EmulatedDeviceObject> GeneratePS4EmulatedDeviceObjects() {
  std::vector<EmulatedDeviceObject> result;
  // Axes
  result.push_back({.name = "X Axis",
                    .guid = GUID_XAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 1,
                    .offset = 12,
                    .mapped_object = AxisType::LStickX});
  result.push_back({.name = "Y Axis",
                    .guid = GUID_YAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 2,
                    .offset = 8,
                    .mapped_object = AxisType::LStickY});
  result.push_back({.name = "Z Axis",
                    .guid = GUID_ZAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 3,
                    .offset = 4,
                    .mapped_object = AxisType::RStickX});
  result.push_back({.name = "Z Rotation",
                    .guid = GUID_RzAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 6,
                    .offset = 0,
                    .mapped_object = AxisType::RStickY});
  result.push_back({.name = "X Rotation",
                    .guid = GUID_RxAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 4,
                    .offset = 28,
                    .mapped_object = AxisType::LTrigger});
  result.push_back({.name = "Y Rotation",
                    .guid = GUID_RyAxis,
                    .type = DIDFT_ABSAXIS,
                    .flags = DIDOI_ASPECTPOSITION,
                    .instance_id = 5,
                    .offset = 24,
                    .mapped_object = AxisType::RTrigger});

  // Buttons
  result.push_back({.name = "Button 1",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 1,
                    .offset = 220,
                    .mapped_object = ButtonType::Square});
  result.push_back({.name = "Button 2",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 2,
                    .offset = 221,
                    .mapped_object = ButtonType::Cross});
  result.push_back({.name = "Button 3",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 3,
                    .offset = 222,
                    .mapped_object = ButtonType::Circle});
  result.push_back({.name = "Button 4",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 4,
                    .offset = 223,
                    .mapped_object = ButtonType::Triangle});
  result.push_back({.name = "Button 5",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 5,
                    .offset = 224,
                    .mapped_object = ButtonType::L1});
  result.push_back({.name = "Button 6",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 6,
                    .offset = 225,
                    .mapped_object = ButtonType::R1});
  result.push_back({.name = "Button 7",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 7,
                    .offset = 226,
                    .mapped_object = ButtonType::L2});
  result.push_back({.name = "Button 8",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 8,
                    .offset = 227,
                    .mapped_object = ButtonType::R2});
  result.push_back({.name = "Button 9",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 9,
                    .offset = 228,
                    .mapped_object = ButtonType::Share});
  result.push_back({.name = "Button 10",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 10,
                    .offset = 229,
                    .mapped_object = ButtonType::Options});
  result.push_back({.name = "Button 11",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 11,
                    .offset = 230,
                    .mapped_object = ButtonType::L3});
  result.push_back({.name = "Button 12",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 12,
                    .offset = 231,
                    .mapped_object = ButtonType::R3});
  result.push_back({.name = "Button 13",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 13,
                    .offset = 232,
                    .mapped_object = ButtonType::PlayStation});
  result.push_back({.name = "Button 14",
                    .guid = GUID_Button,
                    .type = DIDFT_PSHBUTTON,
                    .flags = 0,
                    .instance_id = 14,
                    .offset = 233,
                    .mapped_object = ButtonType::Trackpad});

  // D-Pad
  result.push_back({.name = "Hat Switch",
                    .guid = GUID_POV,
                    .type = DIDFT_POV,
                    .flags = 0,
                    .instance_id = 1,
                    .offset = 16,
                    .mapped_object = PovType::DPad});

  for (auto& object : result) {
    object.type |= DIDOI_POLLED;
  }
  return result;
}

}  // namespace dhc
