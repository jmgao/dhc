use std::fmt;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct Axis(f32);

impl Axis {
  pub fn get(self) -> f32 {
    self.0
  }

  pub fn set_value(&mut self, value: f32) {
    if value < 0.0 || value > 1.0 {
      panic!("invalid axis value: {}", value);
    }

    self.0 = value;
  }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub enum AxisType {
  LeftStickX,
  LeftStickY,
  RightStickX,
  RightStickY,
  LeftTrigger,
  RightTrigger,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct Button(bool);

impl Button {
  pub fn get(self) -> bool {
    self.0
  }

  pub fn set_value(&mut self, value: bool) {
    self.0 = value;
  }

  pub fn set(&mut self) {
    self.set_value(true);
  }

  pub fn reset(&mut self) {
    self.set_value(false);
  }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub enum ButtonType {
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
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub enum Hat {
  Neutral,
  North,
  NorthEast,
  East,
  SouthEast,
  South,
  SouthWest,
  West,
  NorthWest,
}

impl Default for Hat {
  fn default() -> Hat {
    Hat::Neutral
  }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub enum HatType {
  DPad,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct DeviceInputs {
  pub axis_left_stick_x: Axis,
  pub axis_left_stick_y: Axis,

  pub axis_right_stick_x: Axis,
  pub axis_right_stick_y: Axis,

  pub axis_left_trigger: Axis,
  pub axis_right_trigger: Axis,

  pub hat_dpad: Hat,

  /// Start/Options
  pub button_start: Button,

  /// Back/Share
  pub button_select: Button,

  /// Xbox/PS
  pub button_home: Button,

  /// Y/△
  pub button_north: Button,

  /// B/○
  pub button_east: Button,

  /// A/✖
  pub button_south: Button,

  /// X/□
  pub button_west: Button,

  pub button_l1: Button,
  pub button_l2: Button,
  pub button_l3: Button,

  pub button_r1: Button,
  pub button_r2: Button,
  pub button_r3: Button,

  pub button_trackpad: Button,
}

impl Default for DeviceInputs {
  fn default() -> DeviceInputs {
    DeviceInputs {
      axis_left_stick_x: Axis(0.5),
      axis_left_stick_y: Axis(0.5),

      axis_right_stick_x: Axis(0.5),
      axis_right_stick_y: Axis(0.5),

      axis_left_trigger: Axis(0.5),
      axis_right_trigger: Axis(0.5),

      hat_dpad: Hat::default(),
      button_start: Button::default(),
      button_select: Button::default(),
      button_home: Button::default(),

      button_north: Button::default(),
      button_east: Button::default(),
      button_south: Button::default(),
      button_west: Button::default(),

      button_l1: Button::default(),
      button_l2: Button::default(),
      button_l3: Button::default(),

      button_r1: Button::default(),
      button_r2: Button::default(),
      button_r3: Button::default(),

      button_trackpad: Button::default(),
    }
  }
}

impl fmt::Display for DeviceInputs {
  fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
    let mut buttons = Vec::new();
    if self.button_north.get() {
      buttons.push("△");
    }
    if self.button_east.get() {
      buttons.push("○");
    }
    if self.button_south.get() {
      buttons.push("✖");
    }
    if self.button_west.get() {
      buttons.push("□");
    }
    if self.button_start.get() {
      buttons.push("Start");
    }
    if self.button_select.get() {
      buttons.push("Select");
    }

    if self.button_l1.get() {
      buttons.push("L1");
    }
    if self.button_l2.get() {
      buttons.push("L2");
    }
    if self.button_l3.get() {
      buttons.push("L3");
    }
    if self.button_r1.get() {
      buttons.push("R1");
    }
    if self.button_r2.get() {
      buttons.push("R2");
    }
    if self.button_r3.get() {
      buttons.push("R3");
    }

    if self.button_home.get() {
      buttons.push("Home");
    }
    if self.button_trackpad.get() {
      buttons.push("Trackpad");
    }

    write!(f, "{:?}", buttons)
  }
}

impl DeviceInputs {
  pub fn reset_buttons(&mut self) {
    self.button_start.reset();
    self.button_select.reset();
    self.button_home.reset();
    self.button_north.reset();
    self.button_east.reset();
    self.button_south.reset();
    self.button_west.reset();
    self.button_l1.reset();
    self.button_l2.reset();
    self.button_l3.reset();
    self.button_r1.reset();
    self.button_r2.reset();
    self.button_r3.reset();
    self.button_trackpad.reset();
  }

  pub fn get_axis(&self, axis_type: AxisType) -> Axis {
    match axis_type {
      AxisType::LeftStickX => self.axis_left_stick_x,
      AxisType::LeftStickY => self.axis_left_stick_y,
      AxisType::RightStickX => self.axis_right_stick_x,
      AxisType::RightStickY => self.axis_right_stick_y,
      AxisType::LeftTrigger => self.axis_left_trigger,
      AxisType::RightTrigger => self.axis_right_trigger,
    }
  }

  pub fn get_button(&self, button_type: ButtonType) -> Button {
    match button_type {
      ButtonType::Start => self.button_start,
      ButtonType::Select => self.button_select,
      ButtonType::Home => self.button_home,
      ButtonType::North => self.button_north,
      ButtonType::East => self.button_east,
      ButtonType::South => self.button_south,
      ButtonType::West => self.button_west,
      ButtonType::L1 => self.button_l1,
      ButtonType::L2 => self.button_l2,
      ButtonType::L3 => self.button_l3,
      ButtonType::R1 => self.button_r1,
      ButtonType::R2 => self.button_r2,
      ButtonType::R3 => self.button_r3,
      ButtonType::Trackpad => self.button_trackpad,
    }
  }

  pub fn get_hat(&self, hat_type: HatType) -> Hat {
    match hat_type {
      HatType::DPad => self.hat_dpad,
    }
  }
}
