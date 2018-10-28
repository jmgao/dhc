#!/bin/bash

set -ex

unset CC
unset CXX

if [ "$1" == "--use-gcc" ]; then
  TOOLCHAIN_PREFIX="toolchain-gcc"
else
  TOOLCHAIN_PREFIX="toolchain-clang"
fi

cd $(dirname $(realpath $0))

[ -e i686 ] && rm -r i686
[ -e x86_64 ] && rm -r x86_64

mkdir i686 x86_64

(cd i686; meson ../.. --cross-file ../../toolchain/${TOOLCHAIN_PREFIX}-i686.txt)
(cd x86_64; meson ../.. --cross-file ../../toolchain/${TOOLCHAIN_PREFIX}-x86_64.txt)
