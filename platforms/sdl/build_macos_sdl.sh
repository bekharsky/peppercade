#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT_DIR/scripts/env.sh"
cd "$ROOT_DIR"

python3 scripts/ascii_resources.py --check

BUILD_DIR="platforms/sdl/build-macos"
APP_DIR="$BUILD_DIR/Peppercade.app"
EXECUTABLE="$APP_DIR/Contents/MacOS/Peppercade"

# Peppercade.app is the sole macOS build product. Remove the legacy loose
# executable so an older build cannot be mistaken for the current app.
rm -f "$BUILD_DIR/Peppercade"
rm -rf "$APP_DIR"
mkdir -p "$APP_DIR/Contents/MacOS" "$APP_DIR/Contents/Resources"

CXX=${CXX:-clang++}
SDL2_CONFIG="${SDL2_CONFIG:-sdl2-config}"
if ! command -v "$SDL2_CONFIG" >/dev/null 2>&1; then
  echo "SDL2 config tool not found: $SDL2_CONFIG" >&2
  echo "Set SDL2_CONFIG in .env." >&2
  exit 1
fi
SDL_CFLAGS="$("$SDL2_CONFIG" --cflags)"
SDL_LIBS="$("$SDL2_CONFIG" --libs)"

$CXX -std=c++11 -O2 -Wall -Wno-deprecated-declarations \
  -DPEPPERCADE_DESKTOP_GL=1 \
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
  -framework OpenGL \
  $SDL_LIBS \
  -o "$EXECUTABLE"

cp platforms/sdl/assets/Peppercade.icns "$APP_DIR/Contents/Resources/Peppercade.icns"
cp platforms/sdl/Info.plist "$APP_DIR/Contents/Info.plist"
chmod +x "$EXECUTABLE"

echo "Built $APP_DIR"
