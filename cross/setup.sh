#!/bin/bash

set -ex

rm -r build.32 build.64
mkdir build.32 build.64

cd build.32
meson .. --cross-file ../cross-x86.txt

cd ../build.64
meson .. --cross-file ../cross-x86_64.txt
