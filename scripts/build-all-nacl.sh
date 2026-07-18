#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

projects=(
  tizen-benchmark
  games/stackfall
  games/brickbound
  platforms/tv
)

for project in "${projects[@]}"; do
  printf '== %s ==\n' "$project"
  "./$project/build_nacl.sh"
done
