#![allow(clippy::collapsible_if)]
#![allow(clippy::manual_range_contains)]
#![allow(clippy::missing_safety_doc)]
#![allow(clippy::uninit_assumed_init)]

#[macro_use]
extern crate log;

#[macro_use]
extern crate lazy_static;

#[macro_use]
extern crate indoc;

extern crate winapi;
use winapi::shared::minwindef::MAX_PATH;
use winapi::um::libloaderapi::{GetModuleFileNameW, GetModuleHandleW};

use parking_lot::Once;

use std::path::PathBuf;
use std::sync::RwLock;

pub mod ffi;

mod config;
use config::Config;

mod logger;

mod input;
pub use input::types::*;

mod unwind;

static ONCE: Once = Once::new();

lazy_static! {
  static ref CONFIG_RESULT: std::io::Result<Config> = {
    let mut path = PathBuf::from(get_executable_path());
    path.pop();
    path.push("dhc.toml");
    Config::read(&path)
  };

  static ref CONFIG: Config = {
    CONFIG_RESULT.as_ref().expect("failed to open or create configuration file").clone()
  };

  static ref CONTEXT: Context = {
    Context::new(CONFIG.device_count, CONFIG.mode == config::EmulationMode::XInput)
  };
}

fn get_executable_path() -> String {
  let process = unsafe { GetModuleHandleW(std::ptr::null_mut()) };
  let mut path = Vec::with_capacity(MAX_PATH);
  unsafe {
    let len = GetModuleFileNameW(process, path.as_mut_ptr(), MAX_PATH as u32);
    assert!(len > 0);
    path.set_len(len as usize);
  }
  String::from_utf16(&path).expect("executable path isn't UTF-16?")
}

pub fn init() {
  ONCE.call_once(|| {
    logger::init(&CONFIG);

    info!(
      "dhc v{}-{} ({}) initialized",
      env!("CARGO_PKG_VERSION"),
      env!("VERGEN_SHA_SHORT"),
      env!("VERGEN_COMMIT_DATE")
    );

    // We need to wait until the logger has been initialized to warn about this.
    if let Err(ref error) = *CONFIG_RESULT {
      warn!("failed to read config: {}", error);
      warn!("falling back to default configuration");
    }
  });
}

#[derive(Clone, Default)]
struct VirtualDeviceState {
  inputs: DeviceInputs,
  binding: Option<input::DeviceId>,
}

struct VirtualDeviceId(usize);

struct RealDeviceState {
  id: input::DeviceId,
  name: String,
  buffer: triple_buffer::Output<DeviceInputs>,
  binding: Option<VirtualDeviceId>,
}

struct State {
  virtual_devices: Vec<VirtualDeviceState>,
  real_devices: Vec<RealDeviceState>,
}

fn find_real_device(real_devices: &[RealDeviceState], id: input::DeviceId) -> Option<usize> {
  for (idx, device) in real_devices.iter().enumerate() {
    if device.id == id {
      return Some(idx);
    }
  }
  None
}

impl State {
  fn new(device_count: usize) -> State {
    State {
      virtual_devices: vec![VirtualDeviceState::default(); device_count],
      real_devices: Vec::new(),
    }
  }

  fn bind_devices(&mut self) {
    let State {
      ref mut virtual_devices,
      ref mut real_devices,
    } = *self;

    // Bind any unbound virtual devices.
    for (vdev_idx, vdev) in virtual_devices.iter_mut().enumerate() {
      if vdev.binding.is_some() {
        continue;
      }

      // Iterate over real devices in reverse order to bind the newest device first.
      let mut bound = false;
      for rdev in real_devices.iter_mut().rev() {
        if rdev.binding.is_some() {
          continue;
        }

        info!("Binding virtual device {} to {} ({:?})", vdev_idx, rdev.name, rdev.id);
        vdev.binding = Some(rdev.id);
        rdev.binding = Some(VirtualDeviceId(vdev_idx));
        bound = true;
        break;
      }

      if !bound {
        // We're out of devices to try to bind.
        break;
      }
    }
  }

  fn unbind_device(&mut self, real_device_idx: usize) {
    let State {
      ref mut virtual_devices,
      ref mut real_devices,
    } = *self;

    let rdev = &mut real_devices[real_device_idx];
    if let Some(VirtualDeviceId(vdev_idx)) = rdev.binding {
      let vdev = &mut virtual_devices[vdev_idx];
      assert_eq!(Some(rdev.id), vdev.binding);
      info!(
        "Unbinding virtual device {} from {} ({:?})",
        vdev_idx, rdev.name, rdev.id
      );
      vdev.binding = None;
      rdev.binding = None;
      vdev.inputs = DeviceInputs::default();
    }
  }

  fn add_device(&mut self, id: input::DeviceId, name: String, buffer: triple_buffer::Output<DeviceInputs>) {
    info!("Device arrived: {} ({:?})", name, id);
    self.real_devices.push(RealDeviceState {
      id,
      name,
      buffer,
      binding: None,
    });
    self.bind_devices();
  }

  fn remove_device(&mut self, id: input::DeviceId) {
    let real_device_idx = find_real_device(&self.real_devices, id).unwrap();
    self.unbind_device(real_device_idx);

    let device_name = &self.real_devices[real_device_idx].name;
    info!("Device removed: {} ({:?})", device_name, id);

    self.real_devices.remove(real_device_idx);
    self.bind_devices();
  }

  fn update(&mut self) {
    let State {
      ref mut virtual_devices,
      ref mut real_devices,
    } = *self;

    for mut vdev in virtual_devices.iter_mut() {
      if let Some(rdev_id) = vdev.binding {
        let real_device_idx = find_real_device(&real_devices, rdev_id).unwrap();
        let rdev = &mut real_devices[real_device_idx];
        vdev.inputs = *rdev.buffer.read();
      }
    }
  }
}

pub struct Context {
  input: input::Context,
  state: RwLock<State>,
  device_count: usize,
  xinput_enabled: bool,
}

impl Context {
  fn new(device_count: usize, xinput_enabled: bool) -> Context {
    let ctx = input::Context::new();
    ctx.register_device_type(input::RawInputDeviceType::Joystick);
    ctx.register_device_type(input::RawInputDeviceType::GamePad);
    Context {
      input: ctx,
      state: RwLock::new(State::new(device_count)),
      device_count,
      xinput_enabled,
    }
  }

  pub fn instance() -> &'static Context {
    &CONTEXT
  }

  pub fn device_count(&self) -> usize {
    self.device_count
  }

  pub fn xinput_enabled(&self) -> bool {
    self.xinput_enabled
  }

  pub fn device_state(&self, idx: usize) -> DeviceInputs {
    trace!("Context::device_state({})", idx);
    let state = self.state.read().unwrap();
    state.virtual_devices[idx].inputs
  }

  pub fn update(&self) {
    trace!("Context::update()");
    let mut state = self.state.write().unwrap();

    // Check for new devices.
    let events = self.input.get_events();
    for event in events {
      match event {
        input::RawInputEvent::DeviceArrived(description, buffer) => {
          state.add_device(description.device_id, description.device_name, buffer);
        }

        input::RawInputEvent::DeviceRemoved(id) => {
          state.remove_device(id);
        }
      }
    }

    state.update();
  }
}

pub(crate) fn mangle_inputs(inputs: &mut DeviceInputs) {
  if CONFIG.dpad_override {
    if inputs.get_hat(HatType::DPad) != Hat::Neutral {
      inputs.axis_left_stick_x.set_value(0.5);
      inputs.axis_left_stick_y.set_value(0.5);
    }
  }

  if let Some(deadzone_config) = &CONFIG.deadzone {
    if deadzone_config.enabled {
      for ref mut axis in &mut [&mut inputs.axis_left_stick_x, &mut inputs.axis_left_stick_y] {
        let value = axis.get();
        // For convenience, interpolate from [0, 1] to [-1, 1], perform the
        // deadzone operations, and then interpolate back.
        let (sign, magnitude) = if value > 0.5 {
          (1.0, (value - 0.5) * 2.0)
        } else {
          (-1.0, (0.5 - value) * 2.0)
        };

        let magnitude = if magnitude > deadzone_config.threshold {
          1.0
        } else {
          0.0
        };

        axis.set_value(0.5 + 0.5 * sign * magnitude);
      }
    }
  }
}
