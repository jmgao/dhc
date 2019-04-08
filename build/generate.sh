#!/bin/bash

set -ex

unset CC
unset CXX

if [ "$1" == "--use-clang" ]; then
  TOOLCHAIN_PREFIX="toolchain-clang"
else
  TOOLCHAIN_PREFIX="toolchain-gcc"
fi

cd $(dirname $(realpath $0))

[ -e i686 ] && rm -r i686
[ -e x86_64 ] && rm -r x86_64

mkdir i686 x86_64

# HACK: meson complains if we use include_directories on a directory that doesn't exist yet, so...
mkdir i686/include x86_64/include

(cd i686; meson ../.. --cross-file ../../toolchain/${TOOLCHAIN_PREFIX}-i686.txt)
(cd x86_64; meson ../.. --cross-file ../../toolchain/${TOOLCHAIN_PREFIX}-x86_64.txt)
