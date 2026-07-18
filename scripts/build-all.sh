#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

python3 scripts/test_ascii_resources.py
python3 scripts/ascii_resources.py --check
./platforms/sdl/build_macos_sdl.sh
./scripts/build-all-nacl.sh
./platforms/sdl/build_wasm.sh
./scripts/build-pages.sh
./scripts/package-all-tv.sh

echo "All Peppercade targets built successfully."
