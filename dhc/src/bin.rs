extern crate dhc;

fn main() {
  pretty_env_logger::init();
  dhc::init();
  let ctx = dhc::Context::instance();

  loop {
    ctx.update();
    std::thread::sleep(std::time::Duration::from_millis(3000));
  }
}
