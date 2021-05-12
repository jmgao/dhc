// HACK: Debian's mingw64 uses sjlj exceptions in its i686 ABI, but rust's
// libstd uses DWARF unwinding which depends on libunwind, even if our library
// is built with panic = "abort".

#[cfg(target_arch = "x86")]
#[no_mangle]
pub extern "C" fn _Unwind_Resume() {
  panic!();
}
