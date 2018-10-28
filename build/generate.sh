#!/bin/bash

set -ex

unset CC
unset CXX

cd $(dirname $(realpath $0))

[ -e i686 ] && rm -r i686
[ -e x86_64 ] && rm -r x86_64

mkdir i686 x86_64

(cd i686; meson ../.. --cross-file ../../toolchain/toolchain-gcc-i686.txt)
(cd x86_64; meson ../.. --cross-file ../../toolchain/toolchain-gcc-x86_64.txt)
