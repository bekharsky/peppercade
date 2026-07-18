#!/usr/bin/env bash

PEPPERCADE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ -f "$PEPPERCADE_ROOT/.env" ]]; then
  set -a
  # shellcheck disable=SC1091
  source "$PEPPERCADE_ROOT/.env"
  set +a
fi

export PEPPERCADE_ROOT
export TIZEN_CLI="${TIZEN_CLI:-tizen}"
export SDB_CLI="${SDB_CLI:-sdb}"
