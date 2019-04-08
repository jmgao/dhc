project(
  'dhc',
  'cpp',
  default_options: [
    'strip=true',
    'buildtype=minsize',
  ],
)

if target_machine.cpu_family() == 'x86'
  dist_dir = join_paths(meson.current_source_dir(), 'dist', 'i686')
elif target_machine.cpu_family() == 'x86_64'
  dist_dir = join_paths(meson.current_source_dir(), 'dist', 'x86_64')
else
  error('Unknown target CPU "' + target_machine.cpu_family() + '"')
endif

add_project_arguments(
  '-Wall',
  '-Wextra',
  '-Werror',

  '-std=c++17',

  # Ignore some annoying useless warnings.
  '-Wno-unused-parameter',

  # Unfortunately, COM interfaces don't have virtual destructors.
  '-Wno-non-virtual-dtor',

  # Disable some Windows cruft.
  '-DWIN32_LEAN_AND_MEAN',
  '-DNOMINMAX',

  # Enable some Windows cruft.
  '-D_WIN32_WINNT=0x0600',
  language: 'cpp',
)

if meson.get_compiler('cpp').get_id() == 'clang'
  add_global_arguments(
    '-Wthread-safety',
    '-Wthread-safety-negative',
    language: 'cpp'
  )
endif

add_project_link_arguments(
  '-Wl,--enable-stdcall-fixup',
  '-static-libgcc',
  '-static-libstdc++',

  language: 'cpp',
)

cargo_script = find_program('build/cargo.sh')
dhc = custom_target(
  'dhc.dll',
  build_by_default: true,
  build_always_stale: true,
  console: true,
  output: ['dhc.dll'],
  input: ['dhc/Cargo.toml'],
  command: [cargo_script, target_machine.cpu_family(), '@INPUT@', '@OUTPUT@'],
  install: true,
  install_dir: dist_dir,
)

cbindgen_script = find_program('build/cbindgen.sh')
dhc_h = custom_target(
  'dhc.h',
  output: ['dhc.h'],
  build_by_default: true,
  build_always_stale: true,
  input: ['dhc/Cargo.toml'],
  command: [cbindgen_script, '@INPUT@', '@OUTPUT@'],
)

include_dir = include_directories('include')

dinput8 = shared_library(
  'dinput8',
  name_prefix: '',

  include_directories: ['dhc/include', include_dir],
  link_args: [dhc.full_path()],
  link_depends: dhc,

  sources: [
    'dinput8/dinput.cpp',
    'dinput8/ps4.cpp',
    'dinput8/utils.cpp',
    dhc_h,
  ],
  vs_module_defs: 'dinput8/dinput8.def',

  install: true,
  install_dir: dist_dir,
)

xinput1_3 = shared_library(
  'xinput1_3',
  name_prefix: '',

  include_directories: ['dhc/include', include_dir],
  link_args: [dhc.full_path()],
  link_depends: dhc,

  sources: [
    'xinput1_3/xinput1_3.cpp',
    dhc_h,
  ],
  vs_module_defs: 'xinput1_3/xinput1_3.def',

  install: true,
  install_dir: dist_dir,
)
