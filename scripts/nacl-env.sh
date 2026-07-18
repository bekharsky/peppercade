#!/usr/bin/env bash

# shellcheck disable=SC1091
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/env.sh"

if [[ -z "${NACL_SDK_ROOT:-}" ]]; then
  echo "NACL_SDK_ROOT is not set. Copy .env.example to .env and configure it." >&2
  exit 1
fi

NACL_CXX="$NACL_SDK_ROOT/toolchain/mac_arm_newlib/bin/arm-nacl-g++"
NACL_STRIP="$NACL_SDK_ROOT/toolchain/mac_arm_newlib/bin/arm-nacl-strip"

if [[ ! -x "$NACL_CXX" ||
      ! -f "$NACL_SDK_ROOT/include/ppapi/c/pp_errors.h" ||
      ! -f "$NACL_SDK_ROOT/toolchain/mac_arm_newlib/arm-nacl/include/stdint.h" ]]; then
  echo "NaCl SDK is incomplete or missing: $NACL_SDK_ROOT" >&2
  echo "Set NACL_SDK_ROOT in .env to a Samsung Pepper 47 SDK." >&2
  exit 1
fi

export NACL_CXX NACL_STRIP
