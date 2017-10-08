project('dhc', 'cpp', default_options: ['cpp_std=c++1z'])

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

dhc = shared_library(
  'dhc',
  sources: [
    'dhc/dhc.cpp',
    'dhc/dinput.cpp',
    'dhc/logging.cpp',
    'dhc/utils.cpp',
    'dhc/xinput.cpp',
  ],
  cpp_args: '-DDHC_EXPORTS'
)

dinput8 = shared_library(
  'dinput8',
  link_with: [dhc],
  sources: ['dinput8/dinput8.cpp'],
  vs_module_defs: 'dinput8/dinput8.def',
)

xinput1_3 = shared_library(
  'xinput1_3',
  link_with: [dhc],
  sources: ['xinput1_3/xinput1_3.cpp'],
  vs_module_defs: 'xinput1_3/xinput1_3.def',
)
