use winapi::shared::minwindef::MAX_PATH;
use winapi::um::sysinfoapi::GetSystemDirectoryW;

use rusty_xinput::XInputHandle;

use crate::input::types::*;
use crate::input::XInputDeviceId;

lazy_static! {
  static ref XINPUT_HANDLE: XInputHandle = open_xinput();
}

fn get_system_directory() -> String {
  let mut vec = Vec::with_capacity(MAX_PATH);
  unsafe {
    let len = GetSystemDirectoryW(vec.as_mut_ptr(), vec.capacity() as u32);
    assert!(len > 0);
    vec.set_len(len as usize);
  }
  String::from_utf16(&vec).unwrap()
}

fn open_xinput() -> XInputHandle {
  let mut system = get_system_directory();
  system.push_str("\\xinput1_3.dll");
  XInputHandle::load(&system).expect("failed to load xinput")
}

pub fn read_xinput(id: XInputDeviceId) -> Option<DeviceInputs> {
  if let Ok(state) = (*XINPUT_HANDLE).get_state(id.0 as u32) {
    let mut inputs = DeviceInputs::default();
    inputs.button_north.set_value(state.north_button());
    inputs.button_east.set_value(state.east_button());
    inputs.button_south.set_value(state.south_button());
    inputs.button_west.set_value(state.west_button());
    inputs.button_start.set_value(state.start_button());
    inputs.button_select.set_value(state.select_button());
    inputs.button_l1.set_value(state.left_shoulder());
    inputs.button_r1.set_value(state.right_shoulder());
    inputs.button_l2.set_value(state.left_trigger_bool());
    inputs.button_r2.set_value(state.right_trigger_bool());
    inputs.button_l3.set_value(state.left_thumb_button());
    inputs.button_r3.set_value(state.right_thumb_button());

    // TODO: Use raw?
    // These values are in the range [-1, 1], ours are [0, 1].
    // Also, the Y axis is upside down.
    let (l_x, l_y) = state.left_stick_normalized();
    inputs.axis_left_stick_x.set_value((l_x + 1.0) / 2.0);
    inputs.axis_left_stick_y.set_value((1.0 - l_y) / 2.0);

    let (r_x, r_y) = state.right_stick_normalized();
    inputs.axis_right_stick_x.set_value((r_x + 1.0) / 2.0);
    inputs.axis_right_stick_y.set_value((1.0 - r_y) / 2.0);

    // TODO: Forward the analog trigger values as well?

    let mut hat_x = 0;
    let mut hat_y = 0;
    if state.arrow_up() {
      hat_y -= 1;
    }

    if state.arrow_down() {
      hat_y += 1;
    }

    if state.arrow_right() {
      hat_x += 1;
    }

    if state.arrow_left() {
      hat_x -= 1;
    }

    inputs.hat_dpad = match (hat_x, hat_y) {
      (-1, -1) => Hat::NorthWest,
      (-1, 0) => Hat::West,
      (-1, 1) => Hat::SouthWest,
      (0, -1) => Hat::North,
      (0, 0) => Hat::Neutral,
      (0, 1) => Hat::South,
      (1, -1) => Hat::NorthEast,
      (1, 0) => Hat::East,
      (1, 1) => Hat::SouthEast,
      _ => panic!("impossible"),
    };

    Some(inputs)
  } else {
    None
  }
}
