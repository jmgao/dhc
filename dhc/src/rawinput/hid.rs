use std::ffi::CString;
use std::fmt::Write;
use std::io;

use winapi::shared::hidpi::{
  HidP_GetCaps, HidP_GetLinkCollectionNodes, HidP_GetUsageValue, HidP_GetUsages, HidP_GetValueCaps,
  HidP_MaxUsageListLength,
};
use winapi::shared::hidpi::{
  HidP_Input, HIDP_STATUS_INCOMPATIBLE_REPORT_ID, HIDP_STATUS_SUCCESS, HIDP_STATUS_USAGE_NOT_FOUND,
};
use winapi::shared::hidpi::{
  HIDP_CAPS, HIDP_LINK_COLLECTION_NODE, HIDP_VALUE_CAPS, PHIDP_PREPARSED_DATA,
};
use winapi::shared::hidsdi::{
  HidD_FreePreparsedData, HidD_GetManufacturerString, HidD_GetPreparsedData, HidD_GetProductString,
  HidD_GetSerialNumberString,
};
use winapi::shared::minwindef::UINT;
use winapi::shared::ntdef::HANDLE;
use winapi::um::fileapi::{CreateFileA, OPEN_EXISTING};
use winapi::um::handleapi::CloseHandle;
use winapi::um::handleapi::INVALID_HANDLE_VALUE;
use winapi::um::winnt::{FILE_SHARE_READ, FILE_SHARE_WRITE};
use winapi::um::winuser::*;

use crate::rawinput::{DeviceDescription, DeviceId, DeviceInputs, Hat};

const USAGE_PAGE_BUTTON: u16 = 9;

const USAGE_X: u16 = 0x30;
const USAGE_Y: u16 = 0x31;
const USAGE_Z: u16 = 0x32;

const USAGE_RX: u16 = 0x33;
const USAGE_RY: u16 = 0x34;
const USAGE_RZ: u16 = 0x35;

const USAGE_HAT: u16 = 0x39;

const MAX_BUTTONS: usize = 32;

enum DeviceType {
  PS4,
  PS3,
  Generic,
  Xinput,
}

struct HidPreparsedData {
  ptr: PHIDP_PREPARSED_DATA,
}

unsafe impl Send for HidPreparsedData {}

impl Drop for HidPreparsedData {
  fn drop(&mut self) {
    unsafe { HidD_FreePreparsedData(self.raw()) };
  }
}

impl HidPreparsedData {
  pub fn new(data: PHIDP_PREPARSED_DATA) -> HidPreparsedData {
    HidPreparsedData { ptr: data }
  }

  pub fn raw(&self) -> PHIDP_PREPARSED_DATA {
    self.ptr
  }

  fn get_caps(&self) -> HIDP_CAPS {
    let mut caps = unsafe { std::mem::uninitialized() };
    let rc = unsafe { HidP_GetCaps(self.raw(), &mut caps) };
    assert_eq!(HIDP_STATUS_SUCCESS, rc);
    caps
  }

  fn get_value_caps(&self) -> Vec<HIDP_VALUE_CAPS> {
    let caps = self.get_caps();
    let mut vec = Vec::with_capacity(caps.NumberInputValueCaps as usize);
    unsafe {
      let mut len = caps.NumberInputValueCaps;
      let rc = HidP_GetValueCaps(HidP_Input, vec.as_mut_ptr(), &mut len, self.raw());
      assert_eq!(HIDP_STATUS_SUCCESS, rc);
      vec.set_len(len as usize);
    }
    vec
  }

  fn get_link_collection_nodes(&self) -> Vec<HIDP_LINK_COLLECTION_NODE> {
    let mut result = Vec::with_capacity(128);
    unsafe {
      let mut size = result.capacity() as u32;
      let rc = HidP_GetLinkCollectionNodes(result.as_mut_ptr(), &mut size, self.raw());
      assert_eq!(HIDP_STATUS_SUCCESS, rc);
      result.set_len(size as usize);
      trace!("nodes: {}", size);
    }
    result
  }

  pub fn get_button_count(&self) -> usize {
    unsafe { HidP_MaxUsageListLength(HidP_Input, USAGE_PAGE_BUTTON, self.raw()) as usize }
  }

  fn get_buttons(&self, data: &[u8]) -> [u16; MAX_BUTTONS] {
    let mut buttons = [0u16; MAX_BUTTONS];
    let mut size = buttons.len() as u32;
    let result = unsafe {
      HidP_GetUsages(
        HidP_Input,
        USAGE_PAGE_BUTTON,
        0,
        buttons.as_mut_ptr(),
        &mut size,
        self.raw(),
        data.as_ptr() as *mut i8,
        data.len() as u32,
      )
    };

    assert_eq!(HIDP_STATUS_SUCCESS, result);
    buttons
  }

  fn get_usage_value(&self, data: &[u8], usage_page: u16, usage: u16) -> i32 {
    let mut result = 0;
    let rc = unsafe {
      HidP_GetUsageValue(
        HidP_Input,
        usage_page,
        0,
        usage,
        &mut result,
        self.raw(),
        data.as_ptr() as *mut i8,
        data.len() as u32,
      )
    };

    assert_ne!(rc, HIDP_STATUS_INCOMPATIBLE_REPORT_ID);
    assert_ne!(rc, HIDP_STATUS_USAGE_NOT_FOUND);
    assert_eq!(rc, HIDP_STATUS_SUCCESS);
    result as i32
  }
}

fn unlerp(x: i32, min: i32, max: i32) -> f32 {
  ((x - min) as f32) / ((max - min) as f32)
}

pub struct HidParser {
  hid: HidPreparsedData,
  device_type: DeviceType,
  value_caps: Vec<HIDP_VALUE_CAPS>,
}

impl HidParser {
  fn new(hid: HidPreparsedData) -> HidParser {
    let mut device_type = DeviceType::Generic;

    if hid.get_button_count() == 14 {
      device_type = DeviceType::PS4;
    } else if hid.get_button_count() == 13 {
      device_type = DeviceType::PS3;
    }

    let caps = hid.get_caps();
    info!(
      "NumberLinkCollectionNodes = {}",
      caps.NumberLinkCollectionNodes
    );
    info!("NumberInputButtonCaps = {}", caps.NumberInputButtonCaps);
    info!("NumberInputValueCaps = {}", caps.NumberInputValueCaps);
    info!("NumberInputDataIndices = {}", caps.NumberInputDataIndices);
    info!("NumberOutputButtonCaps = {}", caps.NumberOutputButtonCaps);
    info!("NumberOutputValueCaps = {}", caps.NumberOutputValueCaps);
    info!("NumberOutputDataIndices = {}", caps.NumberOutputDataIndices);
    info!("NumberFeatureButtonCaps = {}", caps.NumberFeatureButtonCaps);
    info!("NumberFeatureValueCaps = {}", caps.NumberFeatureValueCaps);
    info!(
      "NumberFeatureDataIndices = {}",
      caps.NumberFeatureDataIndices
    );

    let value_caps = hid.get_value_caps();
    for (idx, value_cap) in value_caps.iter().enumerate() {
      info!("Value cap {}:", idx);
      info!("  UsagePage = {:#x}", value_cap.UsagePage);
      info!("  ReportID = {}", value_cap.ReportID);
      info!("  IsAlias = {}", value_cap.IsAlias);
      info!("  BitField = {}", value_cap.BitField);
      info!("  LinkCollection = {}", value_cap.LinkCollection);
      info!("  LinkUsage = {}", value_cap.LinkUsage);
      info!("  LinkUsagePage = {}", value_cap.LinkUsagePage);
      info!("  IsRange = {}", value_cap.IsRange);
      info!("  IsStringRange = {}", value_cap.IsStringRange);
      info!("  IsDesignatorRange = {}", value_cap.IsDesignatorRange);
      if value_cap.IsRange == 0 {
        let cap = unsafe { value_cap.u.NotRange() };
        info!("  Usage = {:#x}", cap.Usage);
      }
    }

    let link_collection_nodes = hid.get_link_collection_nodes();
    for node in &link_collection_nodes {
      info!(
        "node = LinkUsage({}), LinkUsagePage({}), NumberOfChildren({})",
        node.LinkUsage, node.LinkUsagePage, node.NumberOfChildren
      );
    }

    HidParser {
      hid,
      device_type,
      value_caps,
    }
  }

  fn new_xinput(hid: HidPreparsedData) -> HidParser {
    let value_caps = hid.get_value_caps();
    HidParser {
      hid,
      device_type: DeviceType::Xinput,
      value_caps,
    }
  }

  pub fn parse(&self, data: &[u8]) -> DeviceInputs {
    match self.device_type {
      DeviceType::PS4 => self.parse_ps4(data),

      DeviceType::PS3 => {
        // PS3 appears to be a strict subset of the PS4.
        self.parse_ps4(data)
      }

      DeviceType::Generic => {
        // TODO: Be smarter at parsing generic inputs?
        self.parse_ps4(data)
      }

      DeviceType::Xinput => self.parse_xinput(data),
    }
  }

  pub fn parse_ps4(&self, data: &[u8]) -> DeviceInputs {
    let mut result = DeviceInputs::default();

    let buttons = self.hid.get_buttons(data);
    for button in &buttons {
      match button {
        0 => break,
        1 => result.button_west.set(),
        2 => result.button_south.set(),
        3 => result.button_east.set(),
        4 => result.button_north.set(),
        5 => result.button_l1.set(),
        6 => result.button_r1.set(),
        7 => result.button_l2.set(),
        8 => result.button_r2.set(),
        9 => result.button_select.set(),
        10 => result.button_start.set(),
        11 => result.button_l3.set(),
        12 => result.button_r3.set(),
        13 => result.button_home.set(),
        14 => result.button_trackpad.set(),
        _ => {}
      }
    }

    for value_cap in &self.value_caps {
      let logical_min = value_cap.LogicalMin;
      let logical_max = value_cap.LogicalMax;

      let usage = unsafe { value_cap.u.NotRange().Usage };
      let value = self.hid.get_usage_value(data, value_cap.UsagePage, usage);

      let unlerped = unlerp(value, logical_min, logical_max);
      match usage {
        USAGE_X => result.axis_left_stick_x.set_value(unlerped),
        USAGE_Y => result.axis_left_stick_y.set_value(unlerped),
        USAGE_Z => result.axis_right_stick_x.set_value(unlerped),
        USAGE_RZ => result.axis_left_stick_y.set_value(unlerped),
        USAGE_RX => result.axis_left_trigger.set_value(unlerped),
        USAGE_RY => result.axis_right_trigger.set_value(unlerped),
        USAGE_HAT => {
          let direction = match value {
            0 => Hat::North,
            1 => Hat::NorthEast,
            2 => Hat::East,
            3 => Hat::SouthEast,
            4 => Hat::South,
            5 => Hat::SouthWest,
            6 => Hat::West,
            7 => Hat::NorthWest,
            _ => Hat::Neutral,
          };
          result.hat_dpad = direction;
        }

        _ => continue,
      }
    }

    result
  }

  fn parse_xinput(&self, data: &[u8]) -> DeviceInputs {
    let mut result = DeviceInputs::default();

    let buttons = self.hid.get_buttons(data);
    for button in &buttons {
      match button {
        0 => break,
        1 => result.button_south.set(),
        2 => result.button_east.set(),
        3 => result.button_west.set(),
        4 => result.button_north.set(),
        5 => result.button_l1.set(),
        6 => result.button_r1.set(),
        7 => result.button_select.set(),
        8 => result.button_start.set(),
        9 => result.button_l3.set(),
        10 => result.button_r3.set(),
        _ => {}
      }
    }

    for value_cap in &self.value_caps {
      let logical_min = value_cap.LogicalMin;
      let mut logical_max = value_cap.LogicalMax;

      if logical_max == -1 {
        logical_max = 65535
      }

      let usage = unsafe { value_cap.u.NotRange().Usage };
      let value = self.hid.get_usage_value(data, value_cap.UsagePage, usage);

      let unlerped = unlerp(value, logical_min, logical_max);
      match usage {
        USAGE_X => result.axis_left_stick_x.set_value(unlerped),
        USAGE_Y => result.axis_left_stick_y.set_value(unlerped),
        USAGE_RX => result.axis_right_stick_x.set_value(unlerped),
        USAGE_RY => result.axis_right_stick_y.set_value(unlerped),
        USAGE_HAT => {
          let direction = match value {
            0 => Hat::North,
            1 => Hat::NorthEast,
            2 => Hat::East,
            3 => Hat::SouthEast,
            4 => Hat::South,
            5 => Hat::SouthWest,
            6 => Hat::West,
            7 => Hat::NorthWest,
            _ => Hat::Neutral,
          };
          result.hat_dpad = direction;
        }

        _ => {}
      }
    }

    // TODO: Figure out how to parse L2/R2.
    result
  }
}

fn get_rawinput_device_info_impl(device_id: DeviceId, cmd: UINT) -> Vec<u8> {
  let size = match cmd {
    RIDI_DEVICEINFO => std::mem::size_of::<RID_DEVICE_INFO>(),
    _ => {
      let mut received_size = 0;
      let rc = unsafe {
        GetRawInputDeviceInfoA(
          device_id.as_handle(),
          cmd,
          std::ptr::null_mut(),
          &mut received_size,
        )
      };
      assert_eq!(rc, 0);
      received_size as usize
    }
  };

  let mut result = vec![0; size as usize];
  let ptr = result.as_mut_ptr() as *mut std::ffi::c_void;
  let mut dummy = size as u32;
  let rc = unsafe { GetRawInputDeviceInfoA(device_id.as_handle(), cmd, ptr, &mut dummy) };
  assert!(size >= rc as usize);
  result.truncate(rc as usize);
  result
}

fn get_rawinput_device_path(device_id: DeviceId) -> CString {
  let mut result = get_rawinput_device_info_impl(device_id, RIDI_DEVICENAME);

  // Drop the trailing null terminator.
  while !result.is_empty() && *result.last().unwrap() == 0u8 {
    result.pop();
  }

  // Convert to String.
  CString::new(result).unwrap()
}

fn get_rawinput_device_info(device_id: DeviceId) -> RID_DEVICE_INFO {
  let bytes = get_rawinput_device_info_impl(device_id, RIDI_DEVICEINFO);
  #[allow(clippy::cast_ptr_alignment)]
  unsafe {
    std::ptr::read_unaligned(bytes.as_ptr() as *const RID_DEVICE_INFO)
  }
}

fn hid_get_product_string(handle: HANDLE) -> Option<String> {
  let mut buf: Vec<u16> = vec![0; 512];
  let buf_size = buf.len() * std::mem::size_of::<u16>();

  let ptr = buf.as_mut_ptr() as *mut std::ffi::c_void;
  let result = unsafe { HidD_GetProductString(handle, ptr, buf_size as u32) };
  if result == 0 {
    error!("HidD_GetProductString failed");
    None
  } else {
    info!("HidD_GetProductString succeeded");
    Some(
      String::from_utf16_lossy(&buf)
        .trim_end_matches(char::from(0))
        .to_string(),
    )
  }
}

fn hid_get_manufacturer_string(handle: HANDLE) -> Option<String> {
  let mut buf: Vec<u16> = vec![0; 512];
  let buf_size = buf.len() * std::mem::size_of::<u16>();

  let ptr = buf.as_mut_ptr() as *mut std::ffi::c_void;
  let result = unsafe { HidD_GetManufacturerString(handle, ptr, buf_size as u32) };
  if result == 0 {
    error!("HidD_GetManufacturerString failed");
    None
  } else {
    info!("HidD_GetManufacturerString succeeded");
    Some(
      String::from_utf16_lossy(&buf)
        .trim_end_matches(char::from(0))
        .to_string(),
    )
  }
}

fn hid_get_serial_number(handle: HANDLE) -> Option<String> {
  let mut buf: Vec<u8> = vec![0; 512];
  let buf_size = buf.len() * std::mem::size_of::<u8>();

  let ptr = buf.as_mut_ptr() as *mut std::ffi::c_void;
  let result = unsafe { HidD_GetSerialNumberString(handle, ptr, buf_size as u32) };
  if result == 0 {
    error!("HidD_GetSerialNumberString failed");
    None
  } else {
    info!("HidD_GetSerialNumberString succeeded");
    Some(
      String::from_utf8_lossy(&buf)
        .trim_end_matches(char::from(0))
        .to_string(),
    )
  }
}

fn open_rawinput_hid_device(path: CString) -> io::Result<HANDLE> {
  let hid_file = unsafe {
    CreateFileA(
      path.as_ptr(),
      0,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      std::ptr::null_mut(),
      OPEN_EXISTING,
      0,
      std::ptr::null_mut(),
    )
  };

  if hid_file == INVALID_HANDLE_VALUE {
    let err = std::io::Error::last_os_error();
    error!(
      "failed to open HID device at {}: {}",
      path.to_str().unwrap(),
      err
    );
    Err(err)
  } else {
    info!("successfully opened HID device: {}", path.to_str().unwrap());
    Ok(hid_file)
  }
}

fn get_rawinput_device_name(hid_file: HANDLE) -> String {
  let vendor_name = hid_get_manufacturer_string(hid_file);
  let product_name = hid_get_product_string(hid_file);
  let serial_number = hid_get_serial_number(hid_file);

  let mut result = String::new();

  if let Some(vendor) = vendor_name {
    write!(&mut result, "{} ", vendor).unwrap();
  }
  if let Some(product) = product_name {
    write!(&mut result, "{} ", product).unwrap();
  }
  if let Some(serial) = serial_number {
    write!(&mut result, "({}) ", serial).unwrap();
  }

  if result.is_empty() {
    "<unknown>".to_string()
  } else {
    result.pop();
    result
  }
}

fn hid_get_preparsed_data(hid_file: HANDLE) -> io::Result<HidPreparsedData> {
  let mut preparsed_data = std::ptr::null_mut();
  let result = unsafe { HidD_GetPreparsedData(hid_file, &mut preparsed_data) };
  if result == 0 {
    let err = std::io::Error::last_os_error();
    error!("HidD_GetPreparsedData failed: {}", err);
    Err(err)
  } else {
    Ok(HidPreparsedData::new(preparsed_data))
  }
}

fn is_xinput_device_path(path: &CString) -> bool {
  twoway::find_bytes(path.as_bytes(), b"&IG_").is_some()
}

pub fn open_rawinput_device(device_id: DeviceId) -> io::Result<(HidParser, DeviceDescription)> {
  let info = get_rawinput_device_info(device_id);

  let hid_path = get_rawinput_device_path(device_id);
  info!("path = {:?}", hid_path);
  let is_xinput = is_xinput_device_path(&hid_path);
  let hid_file = open_rawinput_hid_device(hid_path)?;
  let device_name = get_rawinput_device_name(hid_file);
  let preparsed_data = hid_get_preparsed_data(hid_file)?;
  unsafe { CloseHandle(hid_file) };

  assert_eq!(RIM_TYPEHID, info.dwType);

  let hid_parser = if is_xinput {
    HidParser::new_xinput(preparsed_data)
  } else {
    HidParser::new(preparsed_data)
  };

  Ok((
    hid_parser,
    DeviceDescription {
      device_id,
      device_name,
    },
  ))
}
