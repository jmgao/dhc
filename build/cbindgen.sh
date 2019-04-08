#!/bin/bash

CARGO_TOML="$1"
HEADER_OUTPUT_PATH="$2"

ROOT=$(dirname "$(realpath "$CARGO_TOML")")/..
OUTPUT_DIR=$(dirname "$(realpath "$HEADER_OUTPUT_PATH")")

mkdir -p ${OUTPUT_DIR}/include/dhc
cbindgen ${ROOT}/dhc -o ${OUTPUT_DIR}/include/dhc/dhc.h
