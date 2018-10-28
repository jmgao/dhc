#!/bin/bash

set -ex

unset CC
unset CXX

cd $(dirname $(realpath $0))
./generate.sh $@
ninja -C i686 install
ninja -C x86_64 install
