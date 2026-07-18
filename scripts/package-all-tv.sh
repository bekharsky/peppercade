#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT_DIR/scripts/env.sh"

if [[ -z "${TIZEN_PROFILE:-}" ]]; then
  echo "TIZEN_PROFILE is not set. Copy .env.example to .env and configure it." >&2
  exit 1
fi
if ! command -v "$TIZEN_CLI" >/dev/null 2>&1; then
  echo "TV packaging CLI not found: $TIZEN_CLI" >&2
  exit 1
fi

targets=(
  tizen-benchmark/web
  games/stackfall/web
  games/brickbound/web
  platforms/tv/web
)

for target in "${targets[@]}"; do
  web_dir="$ROOT_DIR/$target"
  printf '== package %s ==\n' "$target"
  find "$web_dir" -maxdepth 1 -type f -name '*.wgt' -delete
  "$TIZEN_CLI" package -t wgt -s "$TIZEN_PROFILE" -- "$web_dir"
done
