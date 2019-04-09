use std::ffi::CString;
use std::fmt::Write;
use std::io;

use winapi::shared::hidpi::{
  HidP_GetCaps, HidP_GetLinkCollectionNodes, HidP_GetUsageValue, HidP_GetUsages, HidP_GetValueCaps,
  HidP_MaxUsageListLength,
};
use winapi::shared::hidpi::{
  HidP_Input, HIDP_STATUS_BAD_LOG_PHY_VALUES, HIDP_STATUS_BUFFER_TOO_SMALL, HIDP_STATUS_BUTTON_NOT_PRESSED,
  HIDP_STATUS_DATA_INDEX_NOT_FOUND, HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE, HIDP_STATUS_I8042_TRANS_UNKNOWN,
  HIDP_STATUS_INCOMPATIBLE_REPORT_ID, HIDP_STATUS_INTERNAL_ERROR, HIDP_STATUS_INVALID_PREPARSED_DATA,
  HIDP_STATUS_INVALID_REPORT_LENGTH, HIDP_STATUS_INVALID_REPORT_TYPE, HIDP_STATUS_IS_VALUE_ARRAY,
  HIDP_STATUS_NOT_IMPLEMENTED, HIDP_STATUS_NOT_VALUE_ARRAY, HIDP_STATUS_NULL, HIDP_STATUS_REPORT_DOES_NOT_EXIST,
  HIDP_STATUS_SUCCESS, HIDP_STATUS_USAGE_NOT_FOUND, HIDP_STATUS_VALUE_OUT_OF_RANGE,
};
use winapi::shared::hidpi::{HIDP_CAPS, HIDP_LINK_COLLECTION_NODE, HIDP_VALUE_CAPS, PHIDP_PREPARSED_DATA};
use winapi::shared::hidsdi::{
  HidD_FreePreparsedData, HidD_GetManufacturerString, HidD_GetPreparsedData, HidD_GetProductString,
  HidD_GetSerialNumberString,
};
use winapi::shared::minwindef::UINT;
use winapi::shared::ntdef::{HANDLE, NTSTATUS};
use winapi::um::fileapi::{CreateFileA, OPEN_EXISTING};
use winapi::um::handleapi::CloseHandle;
use winapi::um::handleapi::INVALID_HANDLE_VALUE;
use winapi::um::winnt::{FILE_SHARE_READ, FILE_SHARE_WRITE};
use winapi::um::winuser::*;

use crate::input::types::{DeviceInputs, Hat};
use crate::input::{DeviceDescription, DeviceId, DeviceType, RawInputDeviceId};

const USAGE_PAGE_BUTTON: u16 = 9;

const USAGE_X: u16 = 0x30;
const USAGE_Y: u16 = 0x31;
const USAGE_Z: u16 = 0x32;

const USAGE_RX: u16 = 0x33;
const USAGE_RY: u16 = 0x34;
const USAGE_RZ: u16 = 0x35;

const USAGE_HAT: u16 = 0x39;

const MAX_BUTTONS: usize = 32;

struct HidPreparsedData {
  ptr: PHIDP_PREPARSED_DATA,
}

unsafe impl Send for HidPreparsedData {}

impl Drop for HidPreparsedData {
  fn drop(&mut self) {
    unsafe { HidD_FreePreparsedData(self.raw()) };
  }
}

#[derive(Debug)]
pub enum HidPError {
  BadLogPhyValues,
  BufferTooSmall,
  ButtonNotPressed,
  DataIndexNotFound,
  DataIndexOutOfRange,
  I8042TransUnknown,
  IncompatibleReportId,
  InternalError,
  InvalidPreparsedData,
  InvalidReportLength,
  InvalidReportType,
  IsValueArray,
  NotImplemented,
  NotValueArray,
  Null,
  ReportDoesNotExist,
  Success,
  UsageNotFound,
  ValueOutOfRange,
  Unknown(NTSTATUS),
}

impl HidPError {
  fn from_code(code: NTSTATUS) -> HidPError {
    match code {
      HIDP_STATUS_BAD_LOG_PHY_VALUES => HidPError::BadLogPhyValues,
      HIDP_STATUS_BUFFER_TOO_SMALL => HidPError::BufferTooSmall,
      HIDP_STATUS_BUTTON_NOT_PRESSED => HidPError::ButtonNotPressed,
      HIDP_STATUS_DATA_INDEX_NOT_FOUND => HidPError::DataIndexNotFound,
      HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE => HidPError::DataIndexOutOfRange,
      HIDP_STATUS_I8042_TRANS_UNKNOWN => HidPError::I8042TransUnknown,
      HIDP_STATUS_INCOMPATIBLE_REPORT_ID => HidPError::IncompatibleReportId,
      HIDP_STATUS_INTERNAL_ERROR => HidPError::InternalError,
      HIDP_STATUS_INVALID_PREPARSED_DATA => HidPError::InvalidPreparsedData,
      HIDP_STATUS_INVALID_REPORT_LENGTH => HidPError::InvalidReportLength,
      HIDP_STATUS_INVALID_REPORT_TYPE => HidPError::InvalidReportType,
      HIDP_STATUS_IS_VALUE_ARRAY => HidPError::IsValueArray,
      HIDP_STATUS_NOT_IMPLEMENTED => HidPError::NotImplemented,
      HIDP_STATUS_NOT_VALUE_ARRAY => HidPError::NotValueArray,
      HIDP_STATUS_NULL => HidPError::Null,
      HIDP_STATUS_REPORT_DOES_NOT_EXIST => HidPError::ReportDoesNotExist,
      HIDP_STATUS_SUCCESS => HidPError::Success,
      HIDP_STATUS_USAGE_NOT_FOUND => HidPError::UsageNotFound,
      HIDP_STATUS_VALUE_OUT_OF_RANGE => HidPError::ValueOutOfRange,
      _ => HidPError::Unknown(code),
    }
  }
}

impl std::fmt::Display for HidPError {
  fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
    write!(f, "{:?}", self)
  }
}

impl std::error::Error for HidPError {
  fn description(&self) -> &str {
    "HID error"
  }

  fn cause(&self) -> Option<&std::error::Error> {
    None
  }
}

impl From<HidPError> for std::io::Error {
  fn from(err: HidPError) -> Self {
    std::io::Error::new(std::io::ErrorKind::Other, err)
  }
}

fn hidp_result(status: NTSTATUS) -> Result<(), HidPError> {
  if status == HIDP_STATUS_SUCCESS {
    Ok(())
  } else {
    Err(HidPError::from_code(status))
  }
}

impl HidPreparsedData {
  pub fn new(data: PHIDP_PREPARSED_DATA) -> HidPreparsedData {
    HidPreparsedData { ptr: data }
  }

  pub fn raw(&self) -> PHIDP_PREPARSED_DATA {
    self.ptr
  }

  fn get_caps(&self) -> Result<HIDP_CAPS, HidPError> {
    let mut caps = unsafe { std::mem::uninitialized() };
    let rc = unsafe { HidP_GetCaps(self.raw(), &mut caps) };
    let result = hidp_result(rc);
    result.and(Ok(caps))
  }

  fn get_value_caps(&self) -> Result<Vec<HIDP_VALUE_CAPS>, HidPError> {
    let caps = self.get_caps()?;
    let mut vec = Vec::with_capacity(caps.NumberInputValueCaps as usize);
    unsafe {
      let mut len = caps.NumberInputValueCaps;
      let rc = HidP_GetValueCaps(HidP_Input, vec.as_mut_ptr(), &mut len, self.raw());
      let result = hidp_result(rc);
      result.and(Ok({
        vec.set_len(len as usize);
        vec
      }))
    }
  }

  #[allow(dead_code)]
  fn get_link_collection_nodes(&self) -> Result<Vec<HIDP_LINK_COLLECTION_NODE>, HidPError> {
    let mut vec = Vec::with_capacity(128);
    unsafe {
      let mut len = vec.capacity() as u32;
      let rc = HidP_GetLinkCollectionNodes(vec.as_mut_ptr(), &mut len, self.raw());
      let result = hidp_result(rc);
      result.and(Ok({
        vec.set_len(len as usize);
        vec
      }))
    }
  }

  pub fn get_button_count(&self) -> usize {
    unsafe { HidP_MaxUsageListLength(HidP_Input, USAGE_PAGE_BUTTON, self.raw()) as usize }
  }

  fn get_buttons(&self, data: &[u8]) -> Result<[u16; MAX_BUTTONS], HidPError> {
    let mut buttons = [0u16; MAX_BUTTONS];
    let mut size = buttons.len() as u32;
    let rc = unsafe {
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

    hidp_result(rc).and(Ok(buttons))
  }

  fn get_usage_value(&self, data: &[u8], usage_page: u16, usage: u16) -> Result<i32, HidPError> {
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

    hidp_result(rc).and(Ok(result as i32))
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
  fn new(hid: HidPreparsedData) -> Result<HidParser, HidPError> {
    let mut device_type = DeviceType::Generic;

    if hid.get_button_count() == 14 {
      device_type = DeviceType::PS4;
    } else if hid.get_button_count() == 13 {
      device_type = DeviceType::PS3;
    }

    let value_caps = hid.get_value_caps()?;
    for (idx, value_cap) in value_caps.iter().enumerate() {
      debug!("Value cap {}:", idx);
      debug!("  UsagePage = {:#x}", value_cap.UsagePage);
      debug!("  ReportID = {}", value_cap.ReportID);
      debug!("  IsAlias = {}", value_cap.IsAlias);
      debug!("  BitField = {}", value_cap.BitField);
      debug!("  LinkCollection = {}", value_cap.LinkCollection);
      debug!("  LinkUsage = {}", value_cap.LinkUsage);
      debug!("  LinkUsagePage = {}", value_cap.LinkUsagePage);
      debug!("  IsRange = {}", value_cap.IsRange);
      debug!("  IsStringRange = {}", value_cap.IsStringRange);
      debug!("  IsDesignatorRange = {}", value_cap.IsDesignatorRange);
      if value_cap.IsRange == 0 {
        let cap = unsafe { value_cap.u.NotRange() };
        debug!("  Usage = {:#x}", cap.Usage);
      }
    }

    Ok(HidParser {
      hid,
      device_type,
      value_caps,
    })
  }

  fn new_xinput(hid: HidPreparsedData) -> Result<HidParser, HidPError> {
    let value_caps = hid.get_value_caps()?;
    Ok(HidParser {
      hid,
      device_type: DeviceType::XInput,
      value_caps,
    })
  }

  pub fn parse(&self, data: &[u8]) -> Result<DeviceInputs, HidPError> {
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

      DeviceType::XInput => {
        unimplemented!();
      }
    }
  }

  pub fn parse_ps4(&self, data: &[u8]) -> Result<DeviceInputs, HidPError> {
    let mut result = DeviceInputs::default();

    let buttons = self.hid.get_buttons(data)?;
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
      let value = self.hid.get_usage_value(data, value_cap.UsagePage, usage)?;

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

    Ok(result)
  }
}

fn get_rawinput_device_info_impl(device_id: RawInputDeviceId, cmd: UINT) -> Vec<u8> {
  let size = match cmd {
    RIDI_DEVICEINFO => std::mem::size_of::<RID_DEVICE_INFO>(),
    _ => {
      let mut received_size = 0;
      let rc = unsafe { GetRawInputDeviceInfoA(device_id.as_handle(), cmd, std::ptr::null_mut(), &mut received_size) };
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

fn get_rawinput_device_path(device_id: RawInputDeviceId) -> CString {
  let mut result = get_rawinput_device_info_impl(device_id, RIDI_DEVICENAME);

  // Drop the trailing null terminator.
  while !result.is_empty() && *result.last().unwrap() == 0u8 {
    result.pop();
  }

  // Convert to String.
  CString::new(result).unwrap()
}

fn get_rawinput_device_info(device_id: RawInputDeviceId) -> RID_DEVICE_INFO {
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
    let u16_slice = unsafe { std::slice::from_raw_parts(buf.as_ptr() as *const u16, buf.len() / 2) };
    Some(
      String::from_utf16_lossy(u16_slice)
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
    error!("failed to open HID device at {}: {}", path.to_str().unwrap(), err);
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

pub(crate) fn open_rawinput_device(
  device_id: RawInputDeviceId,
) -> io::Result<(HidParser, DeviceType, DeviceDescription)> {
  let info = get_rawinput_device_info(device_id);

  let hid_path = get_rawinput_device_path(device_id);
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
  }?;

  let device_type = hid_parser.device_type;

  Ok((
    hid_parser,
    device_type,
    DeviceDescription {
      device_id: DeviceId::RawInput(device_id),
      device_name,
    },
  ))
}
