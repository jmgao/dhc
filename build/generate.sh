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

(cd i686; 9k ../.. --toolchain ../../toolchain/${TOOLCHAIN_PREFIX}-i686.bfg)
(cd x86_64; 9k ../.. --toolchain ../../toolchain/${TOOLCHAIN_PREFIX}-x86_64.bfg)
