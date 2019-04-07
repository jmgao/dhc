#!/bin/bash

MESON_TARGET="$1"
CARGO_TOML="$2"
DLL_OUTPUT_PATH="$3"

if [[ "$MESON_TARGET" == "x86" ]]; then
  TARGET=i686-pc-windows-gnu
elif [[ "$MESON_TARGET" == "x86_64" ]]; then
  TARGET=x86_64-pc-windows-gnu
else
  echo "Unknown architecture $MESON_TARGET"
  exit 1
fi

ROOT=$(dirname "$(realpath "$CARGO_TOML")")/..
OUTPUT_DIR=$(dirname "$(realpath "$DLL_OUTPUT_PATH")")
CARGO_TARGET_DIR=${OUTPUT_DIR}/target

cargo build --manifest-path=$CARGO_TOML --target-dir "$CARGO_TARGET_DIR" --release --target $TARGET
cp ${CARGO_TARGET_DIR}/${TARGET}/release/dhc.dll "$OUTPUT_DIR"
cp ${CARGO_TARGET_DIR}/${TARGET}/release/dhc.exe "$OUTPUT_DIR"
