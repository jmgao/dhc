fn main() {
  vergen::generate_cargo_keys(vergen::ConstantsFlags::all()).expect("Unable to generate the cargo keys!");
}
