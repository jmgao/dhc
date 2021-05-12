use vergen::{vergen, Config, TimestampKind};

fn main() {
  let mut config = Config::default();
  *config.git_mut().commit_timestamp_kind_mut() = TimestampKind::DateOnly;

  vergen(config).expect("vergen failed");
}
