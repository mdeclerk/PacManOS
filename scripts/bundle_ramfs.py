#!/usr/bin/env python3
from __future__ import annotations

import argparse
import struct
import tempfile
from pathlib import Path

RAMFS_ID_MAX = 128
MAGIC = b"ASB0"
VERSION = 1
RESERVED = 0
HEADER_STRUCT = struct.Struct("<4sHHI")
ENTRY_STRUCT = struct.Struct("<II")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Pack assets into a ramfs blob")
    parser.add_argument("--dir", type=Path)
    parser.add_argument("--out", required=True, type=Path)
    return parser.parse_args()


def collect_files(src_dir: Path | None) -> list[tuple[str, bytes]]:
    if src_dir is None or not src_dir.exists() or not src_dir.is_dir():
        return []

    files: list[Path] = []
    for path in src_dir.rglob("*"):
        if path.is_file():
            files.append(path)

    entries: list[tuple[str, bytes]] = []
    for path in sorted(files):
        rel = path.relative_to(src_dir).as_posix()
        if not rel:
            raise ValueError(f"empty file id for {path}")
        if "\\" in rel:
            raise ValueError(f"file id must use '/': {rel}")

        rel_bytes = rel.encode("utf-8")
        if len(rel_bytes) > RAMFS_ID_MAX:
            raise ValueError(f"file id too long ({len(rel_bytes)} > {RAMFS_ID_MAX}): {rel}")

        entries.append((rel, path.read_bytes()))

    return entries


def build_blob(entries: list[tuple[str, bytes]]) -> bytes:
    out = bytearray()
    out += HEADER_STRUCT.pack(MAGIC, VERSION, RESERVED, len(entries))

    for rel, data in entries:
        rel_bytes = rel.encode("utf-8")
        out += ENTRY_STRUCT.pack(len(rel_bytes), len(data))
        out += rel_bytes
        out += data

    return bytes(out)


def write_atomic(out_path: Path, payload: bytes) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        mode="wb", dir=out_path.parent, prefix=out_path.name + ".", suffix=".tmp", delete=False
    ) as tmp:
        tmp.write(payload)
        tmp.flush()
        tmp_path = Path(tmp.name)

    if out_path.exists() and out_path.read_bytes() == payload:
        tmp_path.unlink()
        return

    tmp_path.replace(out_path)


def main() -> int:
    args = parse_args()
    src_dir = args.dir.resolve() if args.dir is not None else None
    out_path = args.out.resolve()

    entries = collect_files(src_dir)
    payload = build_blob(entries)
    write_atomic(out_path, payload)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
