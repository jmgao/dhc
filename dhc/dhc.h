#pragma once

#include <windows.h>

#include <array>
#include <memory>
#include <string>

#include "dhc/types.h"
#include "dhc/macros.h"

namespace dhc {

constexpr GUID GUID_DHC_P1 = {
  0xdead571c,
  0x4efc,
  0x9fa7,
  {
    0x9a, 0x7e, 0x8d, 0x10,
    0x00, 0x00, 0x00, 0x01
  }
};

constexpr GUID GUID_DHC_P2 = {
  0xdead571c,
  0x4efc,
  0x9fa7,
  {
    0x9a, 0x7e, 0x8d, 0x10,
    0x00, 0x00, 0x00, 0x02
  }
};

struct Device;
struct InputProvider;

struct Context {
  explicit Context(std::unique_ptr<InputProvider> provider);
  static DHC_API observer_ptr<Context> GetInstance();

  DHC_API observer_ptr<Device> GetDevice(size_t id);

 private:
  std::unique_ptr<InputProvider> provider_;
  std::array<std::unique_ptr<Device>, 2> devices_;
};

enum class AxisType {
  LStickX = 0,
  LStickY = 1,
  RStickX = 2,
  RStickY = 3,
  LTrigger = 4,
  RTrigger = 5,
  kAxisTypeCount,
};

DHC_API REFGUID AxisToGUID(AxisType axis_type);

struct Axis {
  Axis() = default;
  Axis(const Axis& copy) = delete;
  Axis(Axis&& move) = delete;

  AxisType type;
  double value;  // [0, 1]
};

using AxisMap = EnumMap<AxisType, Axis, AxisType::kAxisTypeCount>;

enum class ButtonType {
  Square = 0,
  Cross = 1,
  Circle = 2,
  Triangle = 3,
  L1 = 4,
  R1 = 5,
  L2 = 6,
  R2 = 7,
  Share = 8,    // aka Select
  Options = 9,  // aka Start
  L3 = 10,
  R3 = 11,
  PlayStation = 12,
  Trackpad = 13,
  kButtonTypeCount,
};

struct Button {
  Button() = default;
  Button(const Button& copy) = delete;
  Button(Button&& move) = delete;

  ButtonType type;
  bool pressed;
};

using ButtonMap = EnumMap<ButtonType, Button, ButtonType::kButtonTypeCount>;

enum PovType {
  DPad,
  kPovTypeCount,
};

enum class PovState : long {
  Neutral = -1,
  North = 0,
  NorthEast = 4500,
  East = 9000,
  SouthEast = 13500,
  South = 18000,
  SouthWest = 22500,
  West = 27000,
  NorthWest = 31500,
};

struct Pov {
  Pov() = default;
  Pov(const Pov& copy) = delete;
  Pov(Pov&& move) = delete;

  PovType type;
  PovState state;
};

using PovMap = EnumMap<PovType, Pov, PovType::kPovTypeCount>;

struct Device {
  explicit Device(observer_ptr<Context> context, size_t id) : context_(context), id_(id) {
    Reset();
  }
  ~Device() = default;

  using AxisArray = std::array<Axis, static_cast<size_t>(AxisType::kAxisTypeCount)>;
  using ButtonArray = std::array<Button, static_cast<size_t>(ButtonType::kButtonTypeCount)>;

  const AxisMap& Axes() { return axes_; }
  const ButtonMap& Buttons() { return buttons_; }
  const PovMap& Povs() { return povs_; }

  // Reset the state back to neutral.
  DHC_API void Reset();
  DHC_API void Update();

  size_t Id() const { return id_; }

 private:
  observer_ptr<Context> context_;
  observer_ptr<InputProvider> provider_;

  std::vector<observer_ptr<Device>> assigned_devices_;

  size_t id_;
  AxisMap axes_;
  ButtonMap buttons_;
  PovMap povs_;

  friend struct DinputProvider;
};

}
