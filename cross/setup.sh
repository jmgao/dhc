#!/bin/bash

set -ex

unset CC
unset CXX

[ -e build.32 ] && rm -r build.32
[ -e build.64 ] && rm -r build.64

mkdir build.32 build.64

cd build.32
meson ../.. --cross-file ../cross-x86.txt

cd ../build.64
meson ../.. --cross-file ../cross-x86_64.txt
