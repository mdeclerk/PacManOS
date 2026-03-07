#!/usr/bin/env bash
set -euo pipefail

IMAGE=${IMAGE:-pacmanos-buildenv:latest}
ISO_DBG=${ISO_DBG:-out/debug/pacmanos.iso}
ISO_REL=${ISO_REL:-out/release/pacmanos.iso}
QEMU_MEM=${QEMU_MEM:-16M}

usage() {
  cat <<EOF2
Usage: $0 <command>

Commands:
  init       Build the Docker build environment
  bash       Open an interactive shell inside the build environment container.
  editor     Run Pacman level editor.
  
  make-dbg   Build debug ISO (BUILD=debug).
  make-rel   Build release ISO (BUILD=release).
  clean      Remove build outputs via make clean.

  qemu-dbg   Build debug ISO and run with QEMU.
  qemu-gdb   Build debug ISO and run with QEMU paused with GDB stub (-s -S).
  qemu-rel   Build release ISO and run with QEMU.
  
  bochs-dbg  Build debug ISO and run with Bochs.
  bochs-rel  Build release ISO and run with Bochs.

  smoke      Smoke-test all build/game combinations on host QEMU.
EOF2
}

case "${1:-help}" in
  init)       docker image inspect "$IMAGE" &>/dev/null || docker build --target buildenv -t "$IMAGE" . ;;
  bash)       docker run --rm -it -v "$PWD":/pacmanos -w /pacmanos "$IMAGE" /bin/bash ;;
  make-dbg)   docker run --rm -v "$PWD":/pacmanos -w /pacmanos "$IMAGE" make BUILD=debug "${@:2}" ;;
  make-rel)   docker run --rm -v "$PWD":/pacmanos -w /pacmanos "$IMAGE" make BUILD=release "${@:2}" ;;
  clean)      docker run --rm -v "$PWD":/pacmanos -w /pacmanos "$IMAGE" make clean ;;
  editor)     exec python3 -m src.editor ;;
  qemu-dbg)   "$0" make-dbg && qemu-system-i386 -cdrom "$ISO_DBG" -cpu qemu32 -m "$QEMU_MEM" -serial stdio -no-reboot ;;
  qemu-rel)   "$0" make-rel && qemu-system-i386 -cdrom "$ISO_REL" -cpu qemu32 -m "$QEMU_MEM" -serial stdio -no-reboot ;;
  qemu-gdb)   "$0" make-dbg && qemu-system-i386 -cdrom "$ISO_DBG" -cpu qemu32 -m "$QEMU_MEM" -serial stdio -no-reboot -s -S ;;
  bochs-dbg)  "$0" make-dbg && bochs -f bochsrc/bochsrc-dbg -q ;;
  bochs-rel)  "$0" make-rel && bochs -f bochsrc/bochsrc-rel -q ;;
  smoke)      exec "$PWD/scripts/smoke_matrix.sh" "${@:2}" ;;
  *)          usage ;;
esac
