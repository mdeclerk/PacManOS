#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

readonly IMAGE=${IMAGE:-pacmanos-buildenv:latest}
readonly ISO_DBG=${ISO_DBG:-out/debug/pacmanos.iso}
readonly ISO_REL=${ISO_REL:-out/release/pacmanos.iso}
readonly ELF_DBG=${ELF_DBG:-out/debug/engineos/engineos.elf}
readonly QEMU_MEM=${QEMU_MEM:-16M}

usage() {
  cat <<USAGE
Usage: $0 <command>

Commands:
  init          Build the Docker build environment
  bash          Open an interactive shell inside the build environment container.

  editor        Run Pacman level editor.

  make-dbg      Build debug ISO (BUILD=debug).
  make-rel      Build release ISO (BUILD=release).
  clean         Remove build outputs via make clean.

  qemu-dbg      Build debug ISO and run with QEMU.
  qemu-gdb      Build debug ISO and run QEMU / GDB session.
  qemu-gdb-stub Build debug ISO and run QEMU paused with GDB stub (-s -S).
  qemu-rel      Build release ISO and run with QEMU.

  bochs-dbg     Build debug ISO and run with Bochs.
  bochs-rel     Build release ISO and run with Bochs.

  smoke         Smoke-test all build/game combinations on host QEMU.
USAGE
}

require_cmd() {
  command -v "$1" &>/dev/null || { echo "error: $1 is not installed" >&2; exit 1; }
}

cmd_init() {
  require_cmd docker

  # 1. Already available locally?
  if docker image inspect "$IMAGE" &>/dev/null; then
    echo "$IMAGE already found locally."
    return
  fi

  # 2. Try pulling from GHCR
  local owner
  owner=$(git remote get-url origin 2>/dev/null \
    | sed -E 's#(https://github\.com/|git@github\.com:)([^/]+)/.*#\2#') || true
  if [ -n "$owner" ]; then
    local ghcr_image="ghcr.io/${owner}/pacmanos-buildenv:latest"
    echo "Pulling ${ghcr_image} ..."
    if docker pull "$ghcr_image" 2>/dev/null; then
      docker tag "$ghcr_image" "$IMAGE"
      echo "Build environment image pulled from GHCR."
      return
    fi
    echo "Pull failed; building locally instead."
  fi

  # 3. Build from Dockerfile.
  echo "Building build environment image from Dockerfile ..."
  docker build --target buildenv -t "$IMAGE" .
}

cmd_bash() {
  require_cmd docker
  exec docker run --rm -it -v "$PWD:/pacmanos" -w /pacmanos "$IMAGE" /bin/bash
}

cmd_make_dbg() {
  require_cmd docker
  docker run --rm -v "$PWD:/pacmanos" -w /pacmanos "$IMAGE" make BUILD=debug ${@:+"$@"}
}

cmd_make_rel() {
  require_cmd docker
  docker run --rm -v "$PWD:/pacmanos" -w /pacmanos "$IMAGE" make BUILD=release ${@:+"$@"}
}

cmd_clean() {
  require_cmd docker
  docker run --rm -v "$PWD:/pacmanos" -w /pacmanos "$IMAGE" make clean
}

cmd_editor() {
  require_cmd python3
  exec python3 -m src.editor
}

cmd_qemu_dbg() {
  require_cmd qemu-system-i386
  cmd_make_dbg
  exec qemu-system-i386 -cdrom "$ISO_DBG" -cpu qemu32 -m "$QEMU_MEM" \
    -serial stdio -no-reboot
}

cmd_qemu_gdb() {
  require_cmd qemu-system-i386
  require_cmd gdb
  cmd_make_dbg

  local qemu_pid=""
  cleanup() {
    if [[ -n "$qemu_pid" ]] && kill -0 "$qemu_pid" 2>/dev/null; then
      kill "$qemu_pid" 2>/dev/null || true
      wait "$qemu_pid" 2>/dev/null || true
    fi
  }
  trap cleanup EXIT INT TERM

  qemu-system-i386 -cdrom "$ISO_DBG" -cpu qemu32 -m "$QEMU_MEM" \
    -serial file:out/debug/qemu-serial.log -no-reboot -s -S &
  qemu_pid=$!

  gdb "$ELF_DBG" \
    -ex "set disassembly-flavor intel" \
    -ex "set breakpoint pending on" \
    -ex "source scripts/gdb_load_game_syms.py" \
    -ex "target remote localhost:1234"
}

cmd_qemu_gdb_stub() {
  require_cmd qemu-system-i386
  cmd_make_dbg
  echo ">>> QEMU GDB listening on :1234"
  exec qemu-system-i386 -cdrom "$ISO_DBG" -cpu qemu32 -m "$QEMU_MEM" \
    -serial stdio -no-reboot -s -S
}

cmd_qemu_rel() {
  require_cmd qemu-system-i386
  cmd_make_rel
  exec qemu-system-i386 -cdrom "$ISO_REL" -cpu qemu32 -m "$QEMU_MEM" \
    -serial stdio -no-reboot
}

cmd_bochs_dbg() {
  require_cmd bochs
  cmd_make_dbg
  exec bochs -f bochsrc/bochsrc-dbg -q
}

cmd_bochs_rel() {
  require_cmd bochs
  cmd_make_rel
  exec bochs -f bochsrc/bochsrc-rel -q
}

cmd_smoke() {
  require_cmd qemu-system-i386
  exec "$PWD/scripts/smoke_matrix.sh" ${@:+"$@"}
}

case "${1:-help}" in
  init)           cmd_init ;;
  bash)           cmd_bash ;;
  make-dbg)       cmd_make_dbg "${@:2}" ;;
  make-rel)       cmd_make_rel "${@:2}" ;;
  clean)          cmd_clean ;;
  editor)         cmd_editor ;;
  qemu-dbg)       cmd_qemu_dbg ;;
  qemu-gdb)       cmd_qemu_gdb ;;
  qemu-gdb-stub)  cmd_qemu_gdb_stub ;;
  qemu-rel)       cmd_qemu_rel ;;
  bochs-dbg)      cmd_bochs_dbg ;;
  bochs-rel)      cmd_bochs_rel ;;
  smoke)          cmd_smoke "${@:2}" ;;
  *)              usage ;;
esac
