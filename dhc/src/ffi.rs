use crate::*;

use parking_lot::{Once, ONCE_INIT};

#[repr(C)]
#[derive(Copy, Clone, PartialEq)]
pub enum LogLevel {
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  Fatal,
}

impl LogLevel {
  fn to_log(self) -> log::Level {
    match self {
      LogLevel::Trace => log::Level::Trace,
      LogLevel::Debug => log::Level::Debug,
      LogLevel::Info => log::Level::Info,
      LogLevel::Warn => log::Level::Warn,
      LogLevel::Error => log::Level::Error,
      LogLevel::Fatal => log::Level::Error,
    }
  }
}

static ONCE: Once = ONCE_INIT;

#[no_mangle]
pub extern "C" fn dhc_init() {
  ONCE.call_once(|| {
    Context::instance();

    unsafe { AllocConsole() };
    if std::env::var("RUST_LOG").is_err() {
      std::env::set_var("RUST_LOG", "info");
    }
    pretty_env_logger::init();

    info!("dhc initialized");
  });
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
