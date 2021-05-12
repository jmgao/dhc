use std::fs::File;
use std::io;
use std::io::Read;
use std::path::Path;

use serde::{Deserialize, Deserializer};

use crate::logger;

static DEFAULT_CONFIG: &str = indoc!(
  r#"
  # Control the level of verbosity of logging.
  # Valid values are: "trace", "debug", "info", "warn", "error", "fatal"
  log_level = "info"

  # Open a console to output logging.
  console = true

  # Number of devices to emulate.
  device_count = 2

  # The mode of emulation to use.
  # Valid values are "directinput", "xinput".
  # Most games (i.e. any game that supports a PS4 controller) will want "directinput".
  mode = "directinput"

  # Override the left stick with dpad inputs.
  # This flag emulates console behavior for games such as UNDER NIGHT IN-BIRTH on PS4.
  dpad_override = false

  # Deadzone customization.
  # This allows you to set a threshold for left analog stick values.
  # Any x/y values (from 0 to 1) below it are snapped to the center.
  # Any x/y values above it are snapped to the outside.
  # PS4 UNIST appears to use 0.5 as a threshold.
  [deadzone]
  enabled = false
  threshold = 0.5
"#
);

#[repr(C)]
#[derive(Copy, Clone, PartialEq, Debug)]
pub enum EmulationMode {
  DirectInput,
  XInput,
}

impl<'de> Deserialize<'de> for EmulationMode {
  fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
  where
    D: Deserializer<'de>,
  {
    let s = String::deserialize(deserializer)?;
    match s.as_str() {
      "directinput" => Ok(EmulationMode::DirectInput),
      "xinput" => Ok(EmulationMode::XInput),
      _ => Err(serde::de::Error::custom(format!("unknown emulation mode: {}", s))),
    }
  }
}

#[derive(Clone, Deserialize, Debug)]
pub struct Config {
  pub console: bool,
  pub log_level: logger::LogLevel,
  pub device_count: usize,
  pub mode: EmulationMode,
  pub dpad_override: bool,
  pub deadzone: Option<DeadzoneConfig>,
}

#[derive(Clone, Deserialize, Debug)]
pub struct DeadzoneConfig {
  pub enabled: bool,
  pub threshold: f32,
}

impl Config {
  fn parse(s: &str) -> io::Result<Config> {
    toml::from_str(s).map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))
  }

  pub fn read<P: AsRef<Path>>(path: P) -> io::Result<Config> {
    if let Ok(mut file) = File::open(&path) {
      let mut contents = String::new();
      file.read_to_string(&mut contents)?;
      if let Ok(config) = Config::parse(&contents) {
        return Ok(config);
      }
    }

    std::fs::write(&path, DEFAULT_CONFIG)?;
    Ok(Config::parse(DEFAULT_CONFIG).expect("default configuration couldn't be parsed?"))
  }
}
