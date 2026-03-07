#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
TIMEOUT_SECS=${1:-3}
BUILDS=(debug release)

if ! [[ "$TIMEOUT_SECS" =~ ^[0-9]+$ ]] || [[ "$TIMEOUT_SECS" -lt 1 ]]; then
  echo "invalid timeout: $TIMEOUT_SECS" >&2
  exit 1
fi

GAMES=()
while IFS= read -r game; do
  GAMES+=("$game")
done < <(make -s -C "$ROOT_DIR" print-games BUILD=debug)

if [[ "${#GAMES[@]}" -eq 0 ]]; then
  echo "no games returned by 'make print-games'" >&2
  exit 1
fi

for build in "${BUILDS[@]}"; do
  case "$build" in
    debug)
      make_cmd="make-dbg"
      ;;
    release)
      make_cmd="make-rel"
      ;;
  esac

  for game in "${GAMES[@]}"; do
    out_dir="$ROOT_DIR/out/smoke/${game}"
    iso_path="$out_dir/$build/pacmanos.iso"
    echo "==> smoke [$build/$game]"
    if "$ROOT_DIR/buildenv.sh" "$make_cmd" "GAMES=$game" "OUT_DIR=out/smoke/${game}" \
      && "$ROOT_DIR/scripts/smoke_iso.sh" "$iso_path" "$TIMEOUT_SECS"; then
      echo "SMOKE PASS [$build/$game] marker=game_run: timeout=${TIMEOUT_SECS}s iso=$iso_path"
    else
      echo "SMOKE FAIL [$build/$game] marker=game_run: timeout=${TIMEOUT_SECS}s iso=$iso_path" >&2
      exit 1
    fi
  done
done
