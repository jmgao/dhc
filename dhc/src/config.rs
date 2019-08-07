use std::fs::File;
use std::io;
use std::io::Read;
use std::path::Path;

use serde::{Deserialize, Serialize};

use crate::logger;

#[derive(Clone, Serialize, Deserialize, Debug)]
pub struct Config {
  #[serde(default = "default_console")]
  pub console: bool,

  #[serde(default = "default_log_level")]
  pub log_level: logger::LogLevel,

  #[serde(default = "default_device_count")]
  pub device_count: usize,

  #[serde(default = "default_xinput_enabled")]
  pub xinput_enabled: bool,

  pub deadzone: Option<DeadzoneConfig>,
}

#[derive(Clone, Serialize, Deserialize, Debug)]
pub struct DeadzoneConfig {
  pub enabled: bool,
  pub threshold: f32,
}

fn default_console() -> bool {
  true
}

fn default_log_level() -> logger::LogLevel {
  logger::LogLevel::Info
}

fn default_device_count() -> usize {
  2
}

fn default_xinput_enabled() -> bool {
  false
}

impl Default for Config {
  fn default() -> Config {
    Config {
      console: default_console(),
      log_level: default_log_level(),
      device_count: default_device_count(),
      xinput_enabled: default_xinput_enabled(),
      deadzone: Some(DeadzoneConfig {
        enabled: false,
        threshold: 0.5,
      }),
    }
  }
}

impl Config {
  pub fn read<P: AsRef<Path>>(path: P) -> io::Result<Config> {
    if let Ok(mut file) = File::open(&path) {
      let mut contents = String::new();
      file.read_to_string(&mut contents)?;
      toml::from_str(&contents).map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))
    } else {
      // No config file there, let's try to write one.
      let default = Config::default();
      let config = toml::to_string(&default).expect("failed to serialize default configuration");
      std::fs::write(&path, config)?;
      Ok(default)
    }
  }
}
