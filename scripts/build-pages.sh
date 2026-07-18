#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT_DIR/scripts/env.sh"
cd "$ROOT_DIR"

# GitHub's hosted runners do not include the legacy fastcomp SDK this project
# targets. Build the browser bundle locally, commit site/, and let Pages deploy
# the already-verified static files.
OUT_DIR="site" OUTPUT_NAME="index" ./platforms/sdl/build_wasm.sh
rm -f site/index-shell.html

ASSET_VERSION="$(shasum -a 256 site/index.js site/index.wasm | shasum -a 256 | cut -c1-12)"
export ASSET_VERSION
perl -0pi -e '
  s/__PEPPERCADE_ASSET_VERSION__/$ENV{ASSET_VERSION}/g;
  s/src="index\.js"/src="index.js?v=$ENV{ASSET_VERSION}"/g;
  s/src=index\.js/src="index.js?v=$ENV{ASSET_VERSION}"/g;
' site/index.html

echo "GitHub Pages bundle refreshed in site/ (assets $ASSET_VERSION)"
