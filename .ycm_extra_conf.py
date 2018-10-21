import os
import ycm_core

def DirectoryOfThisScript():
    return os.path.dirname(os.path.abspath(__file__))

flags = [
    '-x', 'c++',
    '-std=gnu++1z',
    '-target', 'i686-w64-mingw32',
    '-nostdinc',

    '-I', DirectoryOfThisScript(),
    '-I', '/usr/lib/gcc/i686-w64-mingw32/7.3-posix/include',
    '-I', '/usr/lib/gcc/i686-w64-mingw32/7.3-posix/include/c++',
    '-isystem', '/usr/i686-w64-mingw32/include',

    '-Wall',
    '-Wextra',
    '-Wformat',
    '-Wno-long-long',
    '-Wno-variadic-macros',
    '-Wthread-safety',
]

def FlagsForFile(filename):
    return {
        'flags': flags,
        'do_cache': True
    }
