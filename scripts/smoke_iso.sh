#!/usr/bin/env bash
set -euo pipefail

ISO_PATH=${1:-}
TIMEOUT_SECS=${2:-3}
SUCCESS_MARKER="game_run:"
QEMU_MEM=${QEMU_MEM:-16M}

if [[ -z "$ISO_PATH" ]]; then
  echo "usage: $0 <iso-path> [timeout-seconds]" >&2
  exit 1
fi

if ! [[ "$TIMEOUT_SECS" =~ ^[0-9]+$ ]] || [[ "$TIMEOUT_SECS" -lt 1 ]]; then
  echo "invalid timeout: $TIMEOUT_SECS" >&2
  exit 1
fi

if ! command -v qemu-system-i386 >/dev/null 2>&1; then
  echo "qemu-system-i386 not found in PATH" >&2
  exit 1
fi

if [[ ! -f "$ISO_PATH" ]]; then
  echo "ISO not found: $ISO_PATH" >&2
  exit 1
fi

qemu_pid=""
tmp_log=""
cleanup() {
  if [[ -n "$qemu_pid" ]] && kill -0 "$qemu_pid" 2>/dev/null; then
    kill "$qemu_pid" 2>/dev/null || true
    wait "$qemu_pid" 2>/dev/null || true
  fi
  if [[ -n "$tmp_log" && -f "$tmp_log" ]]; then
    rm -f "$tmp_log"
  fi
}
trap cleanup EXIT INT TERM

tmp_log=$(mktemp)

qemu-system-i386 \
  -cdrom "$ISO_PATH" \
  -m "$QEMU_MEM" \
  -display none \
  -serial stdio \
  -no-reboot \
  -no-shutdown \
  >"$tmp_log" 2>&1 &
qemu_pid=$!

deadline=$((SECONDS + TIMEOUT_SECS))
offset=1

while (( SECONDS < deadline )); do
  if [[ -f "$tmp_log" ]]; then
    while IFS= read -r line; do
      printf '%s\n' "$line"

      if [[ "$line" == *"PANIC:"* ]]; then
        echo "smoke failed: panic detected while booting $ISO_PATH" >&2
        exit 1
      fi

      if [[ "$line" == *"$SUCCESS_MARKER"* ]]; then
        exit 0
      fi
    done < <(sed -n "${offset},\$p" "$tmp_log")

    offset=$(( $(wc -l < "$tmp_log") + 1 ))
  fi

  if ! kill -0 "$qemu_pid" 2>/dev/null; then
    if grep -Fq "$SUCCESS_MARKER" "$tmp_log"; then
      exit 0
    fi
    echo "smoke failed: QEMU exited before reaching success marker for $ISO_PATH" >&2
    cat "$tmp_log" >&2
    exit 1
  fi

  sleep 0.1
done

if grep -Fq "$SUCCESS_MARKER" "$tmp_log"; then
  exit 0
fi

echo "smoke failed: timed out after ${TIMEOUT_SECS}s waiting for '$SUCCESS_MARKER' in $ISO_PATH" >&2
cat "$tmp_log" >&2
exit 1
