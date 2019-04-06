#[macro_use]
extern crate log;
extern crate pretty_env_logger;

#[macro_use]
extern crate lazy_static;

extern crate winapi;
use winapi::um::consoleapi::AllocConsole;

use std::sync::RwLock;

pub mod ffi;

mod input;
pub use input::types::*;

pub fn init() {}

#[derive(Default)]
struct VirtualDeviceState {
  inputs: DeviceInputs,
  binding: Option<input::DeviceId>,
}

// TODO: Make the number of devices configurable?
const VIRTUAL_DEVICE_COUNT: usize = 2;

struct VirtualDeviceId(usize);

struct RealDeviceState {
  id: input::DeviceId,
  name: String,
  buffer: triple_buffer::Output<DeviceInputs>,
  binding: Option<VirtualDeviceId>,
}

#[derive(Default)]
struct State {
  virtual_devices: [VirtualDeviceState; VIRTUAL_DEVICE_COUNT],
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

        info!(
          "Binding virtual device {} to {} ({:?})",
          vdev_idx, rdev.name, rdev.id
        );
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

  fn add_device(
    &mut self,
    id: input::DeviceId,
    name: String,
    buffer: triple_buffer::Output<DeviceInputs>,
  ) {
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
}

lazy_static! {
  static ref CONTEXT: Context = Context::new();
}

impl Context {
  fn new() -> Context {
    let ctx = input::Context::new();
    ctx.register_device_type(input::RawInputDeviceType::Joystick);
    ctx.register_device_type(input::RawInputDeviceType::GamePad);
    Context {
      input: ctx,
      state: RwLock::new(State::default()),
    }
  }

  pub fn instance() -> &'static Context {
    &CONTEXT
  }

  pub fn device_count(&self) -> usize {
    VIRTUAL_DEVICE_COUNT
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
