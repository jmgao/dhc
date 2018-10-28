project(
  'dhc',
  'cpp',
  default_options: [
    'cpp_std=c++1z',
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

dhc = shared_library(
  'dhc',
  name_prefix: '',
  sources: [
    'dhc/dhc.cpp',
    'dhc/backend/dinput.cpp',
    'dhc/frontend/dinput.cpp',
    'dhc/frontend/ps4.cpp',
    'dhc/frontend/xinput.cpp',
    'dhc/logging.cpp',
    'dhc/utils.cpp',
  ],
  cpp_args: '-DDHC_EXPORTS',
  link_args: '-ldinput8',

  install: true,
  install_dir: dist_dir
)

dinput8 = shared_library(
  'dinput8',
  name_prefix: '',
  link_with: [dhc],
  sources: ['dinput8/dinput8.cpp'],
  vs_module_defs: 'dinput8/dinput8.def',

  install: true,
  install_dir: dist_dir
)

xinput1_3 = shared_library(
  'xinput1_3',
  name_prefix: '',
  link_with: [dhc],
  sources: ['xinput1_3/xinput1_3.cpp'],
  vs_module_defs: 'xinput1_3/xinput1_3.def',

  install: true,
  install_dir: dist_dir
)

dhc_test = executable(
  'dhc_test',
  link_with: [dhc],
  sources: ['test/dhc_test.cpp'],
)

dinput_test = executable(
  'dinput_test',
  link_with: [dhc],
  sources: ['test/dinput_test.cpp'],
  link_args: ['-ldinput8', '-ldxguid'],
)
