project(
  'dhc',
  'cpp',
  default_options: [
    'cpp_std=c++1z',
    'strip=true',
    'buildtype=minsize',
  ],
)

add_project_arguments(
  '-Wall',
  '-Wextra',
  '-Werror',

  # Ignore some annoying useless warnings.
  '-Wno-unused-parameter',

  # Unfortunately, COM interfaces don't have virtual destructors.
  '-Wno-non-virtual-dtor',

  language: 'cpp',
)

add_project_link_arguments(
  '-Wl,--enable-stdcall-fixup',

  language: 'cpp',
)

dhc = shared_library(
  'dhc',
  name_prefix: '',
  sources: [
    'dhc/dhc.cpp',
    'dhc/dinput.cpp',
    'dhc/logging.cpp',
    'dhc/utils.cpp',
    'dhc/xinput.cpp',
  ],
  cpp_args: '-DDHC_EXPORTS',

  install: true,
  install_dir: join_paths(meson.current_build_dir(), 'dist'),
)

dinput8 = shared_library(
  'dinput8',
  name_prefix: '',
  link_with: [dhc],
  sources: ['dinput8/dinput8.cpp'],
  vs_module_defs: 'dinput8/dinput8.def',

  install: true,
  install_dir: join_paths(meson.current_build_dir(), 'dist'),
)

xinput1_3 = shared_library(
  'xinput1_3',
  name_prefix: '',
  link_with: [dhc],
  sources: ['xinput1_3/xinput1_3.cpp'],
  vs_module_defs: 'xinput1_3/xinput1_3.def',

  install: true,
  install_dir: join_paths(meson.current_build_dir(), 'dist'),
)
