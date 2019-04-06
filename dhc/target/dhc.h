#include <cstdarg>
#include <cstdint>
#include <cstdlib>

static const uintptr_t MAX_BUTTONS = 32;

static const uint16_t USAGE_HAT = 57;

static const uint16_t USAGE_PAGE_BUTTON = 9;

static const uint16_t USAGE_PAGE_GENERIC_DESKTOP_CONTROLS = 1;

static const uint16_t USAGE_RX = 51;

static const uint16_t USAGE_RY = 52;

static const uint16_t USAGE_RZ = 53;

static const uint16_t USAGE_X = 48;

static const uint16_t USAGE_Y = 49;

static const uint16_t USAGE_Z = 50;

static const uintptr_t VIRTUAL_DEVICE_COUNT = 2;

enum class AxisType {
  LeftStickX,
  LeftStickY,
  RightStickX,
  RightStickY,
  LeftTrigger,
  RightTrigger,
};

enum class ButtonType {
  Start,
  Select,
  Home,
  North,
  East,
  South,
  West,
  L1,
  L2,
  L3,
  R1,
  R2,
  R3,
  Trackpad,
};

enum class Hat {
  Neutral,
  North,
  NorthEast,
  East,
  SouthEast,
  South,
  SouthWest,
  West,
  NorthWest,
};

enum class LogLevel {
  Trace,
  Debug,
  Info,
  Warn,
  Error,
};

struct Axis {
  float _0;
};

struct Button {
  bool _0;
};

struct DeviceInputs {
  Axis axis_left_stick_x;
  Axis axis_left_stick_y;
  Axis axis_right_stick_x;
  Axis axis_right_stick_y;
  Axis axis_left_trigger;
  Axis axis_right_trigger;
  Hat hat_dpad;
  /// Start/Options
  Button button_start;
  /// Back/Share
  Button button_select;
  /// Xbox/PS
  Button button_home;
  /// Y/△
  Button button_north;
  /// B/○
  Button button_east;
  /// A/✖
  Button button_south;
  /// X/□
  Button button_west;
  Button button_l1;
  Button button_l2;
  Button button_l3;
  Button button_r1;
  Button button_r2;
  Button button_r3;
  Button button_trackpad;
};

extern "C" {

Axis dhc_get_axis(DeviceInputs inputs, AxisType axis_type);

Button dhc_get_button(DeviceInputs inputs, ButtonType button_type);

uintptr_t dhc_get_device_count();

DeviceInputs dhc_get_inputs(uintptr_t idx);

void dhc_log(LogLevel level, const uint8_t *msg, uintptr_t msg_len);

void dhc_update();

} // extern "C"
