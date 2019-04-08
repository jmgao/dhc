use crate::*;

use crate::logger::LogLevel;

#[no_mangle]
pub extern "C" fn dhc_init() {
  crate::init();
}

#[no_mangle]
pub unsafe extern "C" fn dhc_log(level: LogLevel, msg: *const u8, msg_len: usize) {
  let string = std::str::from_utf8_unchecked(std::slice::from_raw_parts(msg, msg_len));

  if level == LogLevel::Fatal {
    panic!("{}", string);
  }

  log!(level.to_log(), "{}", string);
}

#[no_mangle]
pub extern "C" fn dhc_log_is_enabled(level: LogLevel) -> bool {
  log_enabled!(level.to_log())
}

#[no_mangle]
pub extern "C" fn dhc_update() {
  Context::instance().update();
}

#[no_mangle]
pub extern "C" fn dhc_get_device_count() -> usize {
  Context::instance().device_count()
}

#[no_mangle]
pub extern "C" fn dhc_get_inputs(index: usize) -> DeviceInputs {
  Context::instance().device_state(index)
}

#[no_mangle]
pub extern "C" fn dhc_get_axis(inputs: DeviceInputs, axis_type: AxisType) -> Axis {
  inputs.get_axis(axis_type)
}

#[no_mangle]
pub extern "C" fn dhc_get_button(inputs: DeviceInputs, button_type: ButtonType) -> Button {
  inputs.get_button(button_type)
}

#[no_mangle]
pub extern "C" fn dhc_get_hat(inputs: DeviceInputs, hat_type: HatType) -> Hat {
  inputs.get_hat(hat_type)
}
