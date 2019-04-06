use std::collections::{HashMap, VecDeque};
use std::fmt;
use std::sync::mpsc::{channel, Sender};

use parking_lot::Mutex;

use winapi::shared::minwindef::{LPARAM, LRESULT, UINT, WPARAM};
use winapi::shared::ntdef::HANDLE;
use winapi::shared::windef::HWND;
use winapi::um::processthreadsapi::{GetCurrentThread, SetThreadPriority};
use winapi::um::winbase::THREAD_PRIORITY_HIGHEST;
use winapi::um::winuser::*;

use hwndloop::*;

use crate::inputs::*;

mod hid;
use hid::*;

#[derive(Debug)]
pub enum DeviceType {
  Joystick,
  GamePad,
}

/// Typed wrapper for a RawInput HANDLE.
#[derive(Copy, Clone, Eq, PartialEq, Hash)]
pub struct DeviceId(pub u64);

impl DeviceId {
  pub fn as_handle(self) -> HANDLE {
    self.0 as HANDLE
  }

  pub fn from_handle(handle: HANDLE) -> DeviceId {
    DeviceId(handle as u64)
  }
}

impl fmt::Debug for DeviceId {
  fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
    write!(f, "DeviceId({:#x})", self.0)
  }
}

pub struct DeviceDescription {
  pub device_id: DeviceId,
  pub device_name: String,
}

impl fmt::Debug for DeviceDescription {
  fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
    write!(f, "{} ({:#x})", self.device_name, self.device_id.0)
  }
}

impl DeviceType {
  fn usage_page(&self) -> u16 {
    self.hid_usage().0
  }

  fn usage(&self) -> u16 {
    self.hid_usage().1
  }

  fn hid_usage(&self) -> (u16, u16) {
    match self {
      DeviceType::Joystick => (0x01, 0x04),
      DeviceType::GamePad => (0x01, 0x05),
    }
  }
}

#[derive(Debug)]
pub enum RawInputEvent {
  DeviceArrived(DeviceDescription, triple_buffer::Output<DeviceInputs>),
  DeviceRemoved(DeviceId),
}

#[derive(Debug)]
enum RawInputCommand {
  RegisterType(DeviceType, Sender<()>),
  UnregisterType(DeviceType, Sender<()>),
  GetEvents(Sender<VecDeque<RawInputEvent>>),
}

struct RawInputManager {
  event_queue: Mutex<VecDeque<RawInputEvent>>,
  devices: HashMap<DeviceId, DeviceState>,
}

struct DeviceState {
  buffer: triple_buffer::Input<DeviceInputs>,
  hid: HidParser,
}

impl DeviceState {
  fn handle_input(&mut self, input: &RAWHID) {
    let size = input.dwSizeHid as usize;
    let count = input.dwCount as usize;
    let ptr = input.bRawData.as_ptr();
    let mut inputs = DeviceInputs::default();
    for i in 0..count {
      let begin = (size * i) as isize;
      let slice = unsafe { std::slice::from_raw_parts(ptr.offset(begin), size) };
      inputs = self.hid.parse(slice);
    }
    self.buffer.write(inputs);
  }
}

impl HwndLoopCallbacks<RawInputCommand> for RawInputManager {
  fn set_up(&mut self, _hwnd: HWND) {
    unsafe { SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST as i32) };
  }

  fn handle_message(&mut self, hwnd: HWND, msg: UINT, w: WPARAM, l: LPARAM) -> LRESULT {
    if msg == WM_INPUT {
      self.handle_device_input(hwnd, l as HRAWINPUT);
    } else if msg == WM_INPUT_DEVICE_CHANGE {
      let device = DeviceId::from_handle(l as HANDLE);
      if w == GIDC_ARRIVAL as usize {
        self.handle_device_arrival(device);
      } else if w == GIDC_REMOVAL as usize {
        self.handle_device_removal(device);
      } else {
        panic!("Unknown argument to WM_INPUT_DEVICE_CHANGE: {}", w);
      }
    }
    unsafe { DefWindowProcA(hwnd, msg, w, l) }
  }

  fn handle_command(&mut self, hwnd: HWND, cmd: RawInputCommand) {
    match cmd {
      RawInputCommand::RegisterType(device_type, reply) => {
        self.cmd_register_device_type(hwnd, device_type, reply);
      }

      RawInputCommand::UnregisterType(device_type, reply) => {
        self.cmd_unregister_device_type(hwnd, device_type, reply);
      }

      RawInputCommand::GetEvents(reply) => {
        self.cmd_get_events(hwnd, reply);
      }
    }
  }
}

#[repr(align(8))]
struct AlignedBuffer {
  #[allow(dead_code)]
  data: [u8; 512],
}

impl RawInputManager {
  fn new() -> RawInputManager {
    RawInputManager {
      event_queue: Mutex::new(VecDeque::new()),
      devices: HashMap::new(),
    }
  }

  fn cmd_register_device_type(&mut self, hwnd: HWND, device_type: DeviceType, reply: Sender<()>) {
    let rid = RAWINPUTDEVICE {
      usUsagePage: device_type.usage_page(),
      usUsage: device_type.usage(),
      dwFlags: RIDEV_INPUTSINK | RIDEV_DEVNOTIFY,
      hwndTarget: hwnd,
    };
    let result =
      unsafe { RegisterRawInputDevices(&rid, 1, std::mem::size_of::<RAWINPUTDEVICE>() as UINT) };
    if result == 0 {
      panic!(
        "RegisterRawInputDevices failed while registering device type {:?}",
        device_type
      );
    }
    reply.send(()).unwrap();
  }

  fn cmd_unregister_device_type(
    &mut self,
    _hwnd: HWND,
    device_type: DeviceType,
    reply: Sender<()>,
  ) {
    let rid = RAWINPUTDEVICE {
      usUsagePage: device_type.usage_page(),
      usUsage: device_type.usage(),
      dwFlags: RIDEV_REMOVE,
      hwndTarget: std::ptr::null_mut(),
    };
    let result =
      unsafe { RegisterRawInputDevices(&rid, 1, std::mem::size_of::<RAWINPUTDEVICE>() as UINT) };
    if result == 0 {
      panic!(
        "RegisterRawInputDevices failed while unregistering device type {:?}",
        device_type
      );
    }
    reply.send(()).unwrap();
  }

  fn cmd_get_events(&mut self, _hwnd: HWND, reply: Sender<VecDeque<RawInputEvent>>) {
    let mut events = self.event_queue.lock();
    let mut empty = VecDeque::new();
    std::mem::swap(&mut *events, &mut empty);
    reply.send(empty).unwrap();
  }

  fn handle_device_input(&mut self, _hwnd: HWND, hrawinput: HRAWINPUT) {
    // TODO: Switch to GetRawInputBuffer?
    let mut buffer = unsafe { std::mem::uninitialized::<AlignedBuffer>() };
    let mut size = std::mem::size_of_val(&buffer) as u32;
    let result = unsafe {
      GetRawInputData(
        hrawinput,
        RID_INPUT,
        &mut buffer as *mut AlignedBuffer as *mut std::ffi::c_void,
        &mut size,
        std::mem::size_of::<RAWINPUTHEADER>() as UINT,
      )
    };

    if result == -1i32 as u32 {
      panic!("GetRawInputData failed to get raw input data");
    }

    let rawinput = unsafe { *(&buffer as *const AlignedBuffer as *const RAWINPUT) };
    let device_id = DeviceId::from_handle(rawinput.header.hDevice);
    let device = self.devices.get_mut(&device_id);
    if device.is_none() {
      return;
    }

    let device = device.unwrap();
    assert_eq!(RIM_TYPEHID, rawinput.header.dwType);
    let data = unsafe { rawinput.data.hid() };
    device.handle_input(data);
  }

  fn handle_device_arrival(&mut self, device_id: DeviceId) {
    let mut queue = self.event_queue.lock();

    let (hid, description) = match open_rawinput_device(device_id) {
      Ok(x) => x,
      Err(_) => return,
    };

    let default_inputs = DeviceInputs::default();
    let (write, read) = triple_buffer::TripleBuffer::new(default_inputs).split();
    let device = DeviceState { buffer: write, hid };

    self.devices.insert(device_id, device);
    queue.push_back(RawInputEvent::DeviceArrived(description, read))
  }

  fn handle_device_removal(&mut self, device_id: DeviceId) {
    let mut queue = self.event_queue.lock();
    queue.push_back(RawInputEvent::DeviceRemoved(device_id));
    self.devices.remove(&device_id);
  }
}

/// Client for the RawInputManager.
pub struct Context {
  eventloop: HwndLoop<RawInputCommand>,
}

impl Context {
  #[allow(clippy::new_without_default)]
  pub fn new() -> Context {
    let manager = RawInputManager::new();
    Context {
      eventloop: HwndLoop::new(Box::new(manager)),
    }
  }

  pub fn register_device_type(&self, device_type: DeviceType) {
    let (tx, rx) = channel();
    let cmd = RawInputCommand::RegisterType(device_type, tx);
    self.eventloop.send_command(cmd);
    rx.recv().unwrap()
  }

  #[allow(dead_code)]
  pub fn unregister_device_type(&self, device_type: DeviceType) {
    let (tx, rx) = channel();
    let cmd = RawInputCommand::UnregisterType(device_type, tx);
    self.eventloop.send_command(cmd);
    rx.recv().unwrap()
  }

  pub fn get_events(&self) -> VecDeque<RawInputEvent> {
    let (tx, rx) = channel();
    let cmd = RawInputCommand::GetEvents(tx);
    self.eventloop.send_command(cmd);
    rx.recv().unwrap()
  }
}