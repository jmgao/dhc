use serde::{Deserialize, Deserializer, Serialize, Serializer};
use winapi::um::consoleapi::AllocConsole;

use crate::config::Config;

use parking_lot::Mutex;

use slog::o;
use slog::Drain;

#[repr(C)]
#[derive(Copy, Clone, PartialEq, Debug)]
pub enum LogLevel {
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  Fatal,
}

impl LogLevel {
  pub(crate) fn to_log(self) -> log::Level {
    match self {
      LogLevel::Trace => log::Level::Trace,
      LogLevel::Debug => log::Level::Debug,
      LogLevel::Info => log::Level::Info,
      LogLevel::Warn => log::Level::Warn,
      LogLevel::Error => log::Level::Error,
      LogLevel::Fatal => log::Level::Error,
    }
  }

  pub(crate) fn to_slog(self) -> slog::Level {
    match self {
      LogLevel::Trace => slog::Level::Trace,
      LogLevel::Debug => slog::Level::Debug,
      LogLevel::Info => slog::Level::Info,
      LogLevel::Warn => slog::Level::Warning,
      LogLevel::Error => slog::Level::Error,
      LogLevel::Fatal => slog::Level::Critical,
    }
  }
}

impl Serialize for LogLevel {
  fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
  where
    S: Serializer,
  {
    serializer.serialize_str(match self {
      LogLevel::Trace => "trace",
      LogLevel::Debug => "debug",
      LogLevel::Info => "info",
      LogLevel::Warn => "warn",
      LogLevel::Error => "error",
      LogLevel::Fatal => "fatal",
    })
  }
}

impl<'de> Deserialize<'de> for LogLevel {
  fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
  where
    D: Deserializer<'de>,
  {
    let s = String::deserialize(deserializer)?;
    match s.as_str() {
      "trace" => Ok(LogLevel::Trace),
      "debug" => Ok(LogLevel::Debug),
      "info" => Ok(LogLevel::Info),
      "warn" => Ok(LogLevel::Warn),
      "error" => Ok(LogLevel::Error),
      "fatal" => Ok(LogLevel::Fatal),
      _ => Err(serde::de::Error::custom(format!("unknown log level: {}", s))),
    }
  }
}

lazy_static! {
  static ref LOGGER: Mutex<Option<slog_scope::GlobalLoggerGuard>> = Mutex::new(None);
}

pub fn init(config: Option<&Config>) {
  let defaults = Config::default();
  let config = config.unwrap_or(&defaults);

  if config.console {
    unsafe { AllocConsole() };
  }

  let console_drain = slog_term::term_compact();
  let file = std::fs::OpenOptions::new()
    .create(true)
    .write(true)
    .truncate(true)
    .open("dhc.log")
    .unwrap();
  let file_decorator = slog_term::PlainDecorator::new(file);
  let file_drain = slog_term::FullFormat::new(file_decorator).build();

  let drain = std::sync::Mutex::new(slog::Duplicate::new(console_drain, file_drain));
  let level = config.log_level.to_slog();
  let filtered = slog::LevelFilter(drain, level);

  let guard = slog_scope::set_global_logger(slog::Logger::root(filtered.fuse(), o!()).into_erased());
  slog_stdlog::init().expect("failed to initialize slog-stdlog");

  let mut lock = LOGGER.lock();
  *lock = Some(guard);

  log_panics::init();
}
