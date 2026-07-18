#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT_DIR/scripts/nacl-env.sh"
cd "$SCRIPT_DIR"

python3 "$ROOT_DIR/scripts/ascii_resources.py" --root "$ROOT_DIR" --check

CXX="$NACL_CXX"
STRIP="$NACL_STRIP"
SDK="$NACL_SDK_ROOT"
CXXFLAGS=(-g -O2 -pthread -MMD -DNDEBUG -Wno-long-long -Wall -std=gnu++11 -I"$SDK/include" -I"$SDK/include/newlib" -I"$ROOT_DIR")

mkdir -p build-nacl

"$CXX" -o build-nacl/brickbound_nacl_arm.o -c src/brickbound_nacl.cc "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/brickbound_game_arm.o -c src/brickbound_game.cc "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/gles2_renderer_arm.o -c "$ROOT_DIR/engine/src/gles2_renderer.cc" "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/ascii_sprite_arm.o -c "$ROOT_DIR/engine/src/ascii_sprite.cc" "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/arcade_style_arm.o -c "$ROOT_DIR/engine/src/arcade_style.cc" "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/arcade_hud_arm.o -c "$ROOT_DIR/engine/src/arcade_hud.cc" "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/pixel_effects_arm.o -c "$ROOT_DIR/engine/src/pixel_effects.cc" "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/debug_overlay_arm.o -c "$ROOT_DIR/engine/src/debug_overlay.cc" "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/arcade_audio_arm.o -c "$ROOT_DIR/engine/src/arcade_audio.cc" "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/nacl_audio_engine_arm.o -c "$ROOT_DIR/engine/src/nacl_audio_engine.cc" "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/game_loop_arm.o -c "$ROOT_DIR/engine/src/game_loop.cc" "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/input_state_arm.o -c "$ROOT_DIR/engine/src/input_state.cc" "${CXXFLAGS[@]}"
"$CXX" -o build-nacl/nacl_input_adapter_arm.o -c "$ROOT_DIR/engine/src/nacl_input_adapter.cc" "${CXXFLAGS[@]}"

"$CXX" -o build-nacl/brickbound_arm_unstripped.nexe \
  build-nacl/brickbound_nacl_arm.o \
  build-nacl/brickbound_game_arm.o \
  build-nacl/gles2_renderer_arm.o \
  build-nacl/ascii_sprite_arm.o \
  build-nacl/arcade_style_arm.o \
  build-nacl/arcade_hud_arm.o \
  build-nacl/pixel_effects_arm.o \
  build-nacl/debug_overlay_arm.o \
  build-nacl/arcade_audio_arm.o \
  build-nacl/nacl_audio_engine_arm.o \
  build-nacl/game_loop_arm.o \
  build-nacl/input_state_arm.o \
  build-nacl/nacl_input_adapter_arm.o \
  -O2 -Wl,-as-needed -pthread \
  -Wl,-Map,build-nacl/brickbound_arm.map \
  -L"$SDK/lib/newlib_arm/Release" \
  -lppapi_gles2 -lppapi_cpp -lppapi

"$STRIP" -o build-nacl/brickbound_arm.nexe build-nacl/brickbound_arm_unstripped.nexe
cp build-nacl/brickbound_arm.nexe web/brickbound_arm.nexe
