#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT_DIR/scripts/env.sh"
cd "$ROOT_DIR"

python3 scripts/ascii_resources.py --check

if [[ -z "${EMSDK_ROOT:-}" ]]; then
  echo "EMSDK_ROOT is not set. Copy .env.example to .env and configure it." >&2
  exit 1
fi
EMXX="${EMXX:-$EMSDK_ROOT/fastcomp/emscripten/em++}"
SDL_CONFIG="${SDL_CONFIG:-$EMSDK_ROOT/fastcomp/emscripten/system/bin/sdl2-config}"
OUT_DIR="${OUT_DIR:-platforms/sdl/build-wasm}"
OUTPUT_NAME="${OUTPUT_NAME:-peppercade}"

if [[ ! -x "$EMXX" ]]; then
  echo "Emscripten compiler not found: $EMXX" >&2
  exit 1
fi

if command -v python >/dev/null 2>&1; then
  EMXX_CMD=("$EMXX")
  SDL_CONFIG_CMD=("$SDL_CONFIG")
else
  PYTHON3="${PYTHON3:-}"
  for candidate in /usr/bin/python3 "$(command -v python3 || true)"; do
    if [[ -x "$candidate" ]] && "$candidate" -c 'import distutils' >/dev/null 2>&1; then
      PYTHON3="$candidate"
      break
    fi
  done
  if [[ -z "$PYTHON3" ]]; then
    echo "Emscripten requires python or python3" >&2
    exit 1
  fi
  # The checked-in SDK is an older release whose launcher has a python shebang.
  EMXX_CMD=("$PYTHON3" "$EMXX")
  SDL_CONFIG_CMD=("$PYTHON3" "$SDL_CONFIG")
fi

mkdir -p "$OUT_DIR"

SDL_CFLAGS="$("${SDL_CONFIG_CMD[@]}" --cflags)"

# The local SDK predates the current emsdk layout; its Binaryen tools live in
# fastcomp/bin rather than a separate binaryen directory.
export EM_BINARYEN_ROOT="${EM_BINARYEN_ROOT:-$EMSDK_ROOT/fastcomp}"
export EM_LLVM_ROOT="${EM_LLVM_ROOT:-$EMSDK_ROOT/fastcomp/bin}"

"${EMXX_CMD[@]}" -std=c++11 -O2 -Wall -Wno-deprecated-declarations \
  -DPEPPERCADE_WASM=1 \
  $SDL_CFLAGS \
  -I. \
  platforms/sdl/src/peppercade_sdl.cc \
  games/stackfall/src/stackfall_core.cc \
  games/stackfall/src/stackfall_patterns.cc \
  games/brickbound/src/brickbound_game.cc \
  games/2048/src/game_2048.cc \
  games/neon-serpent/src/neon_serpent_game.cc \
  games/lightline/src/lightline_game.cc \
  games/dragoban/src/dragoban_game.cc \
  games/bubble-orbit/src/bubble_orbit_game.cc \
  engine/src/gles2_renderer.cc \
  engine/src/ascii_sprite.cc \
  engine/src/arcade_style.cc \
  engine/src/arcade_hud.cc \
  engine/src/game_hub.cc \
  engine/src/debug_overlay.cc \
  engine/src/input_state.cc \
  engine/src/pixel_effects.cc \
  engine/src/arcade_audio.cc \
  engine/src/sdl_audio_engine.cc \
  -s USE_SDL=2 \
  -s USE_WEBGL2=1 \
  -s FULL_ES3=1 \
  -s ENVIRONMENT_MAY_BE_TIZEN=0 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s TOTAL_MEMORY=32MB \
  -s FORCE_FILESYSTEM=1 \
  -s EXPORTED_FUNCTIONS='["_main","_acquire_next_fd","_release_fd","_set_mapped_fd","_is_socket","_get_mapped_fd"]' \
  -s ASSERTIONS=1 \
  --shell-file platforms/sdl/web/peppercade.html \
  -o "$OUT_DIR/$OUTPUT_NAME.html"

cp platforms/sdl/web/peppercade.html "$OUT_DIR/$OUTPUT_NAME-shell.html"
cp platforms/sdl/web/favicon.png "$OUT_DIR/favicon.png"
echo "Built $OUT_DIR/$OUTPUT_NAME.html"
