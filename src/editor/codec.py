from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
import struct
from typing import Iterable, List

MAP_ROWS = 31
MAP_COLS = 28
MAP_TILE_COUNT = MAP_ROWS * MAP_COLS

MAGIC = b"PMAP"
VERSION = 1
NAME_LEN = 32
HEADER_SIZE = 14
RECORD_SIZE = 904

TILE_EMPTY = 0
TILE_DOT = 5
TILE_POWER = 6
TILE_GHOST_GATE = 7
TILE_PACMAN_SPAWN = 8
TILE_GHOST_SPAWN = 9
TILE_WALL_FORCED_BASE = 0x10
TILE_WALL_FORCED_MAX = TILE_WALL_FORCED_BASE + 16

HEADER_STRUCT = struct.Struct("<4sHHHHH")


@dataclass
class LevelRecord:
    name: str
    wall_tint_argb: int
    tiles: bytearray = field(default_factory=lambda: bytearray(MAP_TILE_COUNT))


class LevelsFormatError(ValueError):
    pass


def _is_valid_tile(tile: int) -> bool:
    if TILE_WALL_FORCED_BASE <= tile < TILE_WALL_FORCED_MAX:
        return True
    return tile in {
        TILE_EMPTY,
        TILE_DOT,
        TILE_POWER,
        TILE_GHOST_GATE,
        TILE_PACMAN_SPAWN,
        TILE_GHOST_SPAWN,
    }


def _normalize_name(name: str) -> str:
    if not isinstance(name, str):
        raise LevelsFormatError("level name must be a string")
    name = name.strip()
    if not name:
        raise LevelsFormatError("level name cannot be empty")
    try:
        encoded = name.encode("ascii")
    except UnicodeEncodeError as exc:
        raise LevelsFormatError(f"level name must be ASCII: {name!r}") from exc
    if len(encoded) > NAME_LEN - 1:
        raise LevelsFormatError(f"level name too long (max {NAME_LEN - 1}): {name!r}")
    return name


def validate_levels(records: Iterable[LevelRecord]) -> List[LevelRecord]:
    levels = list(records)
    if not levels:
        raise LevelsFormatError("levels.bin must contain at least one level")

    seen_names = set()
    for idx, rec in enumerate(levels):
        rec.name = _normalize_name(rec.name)
        if rec.name in seen_names:
            raise LevelsFormatError(f"duplicate level name: {rec.name!r}")
        seen_names.add(rec.name)

        rec.wall_tint_argb = 0xFF000000 | (int(rec.wall_tint_argb) & 0x00FFFFFF)

        if not isinstance(rec.tiles, (bytes, bytearray)):
            raise LevelsFormatError(f"level {idx} tiles must be bytes-like")
        if len(rec.tiles) != MAP_TILE_COUNT:
            raise LevelsFormatError(
                f"level {idx} tiles length must be {MAP_TILE_COUNT}, got {len(rec.tiles)}"
            )

        for tile in rec.tiles:
            if not _is_valid_tile(tile):
                raise LevelsFormatError(f"level {idx} has invalid tile value {tile}")

        pacman_spawns = 0
        ghost_spawns = 0
        for tile in rec.tiles:
            if tile == TILE_PACMAN_SPAWN:
                pacman_spawns += 1
            elif tile == TILE_GHOST_SPAWN:
                ghost_spawns += 1

        if pacman_spawns != 1:
            raise LevelsFormatError(f"level {idx} must contain exactly one pacman spawn tile")
        if ghost_spawns != 1:
            raise LevelsFormatError(f"level {idx} must contain exactly one ghost spawn tile")

        if not isinstance(rec.tiles, bytearray):
            rec.tiles = bytearray(rec.tiles)

    return levels


def encode_levels(records: Iterable[LevelRecord]) -> bytes:
    levels = validate_levels(records)
    out = bytearray()
    out += HEADER_STRUCT.pack(MAGIC, VERSION, MAP_ROWS, MAP_COLS, len(levels), RECORD_SIZE)

    for rec in levels:
        name_bytes = rec.name.encode("ascii")
        name_buf = bytearray(NAME_LEN)
        name_buf[: len(name_bytes)] = name_bytes
        out += name_buf
        out += struct.pack("<I", rec.wall_tint_argb)
        out += rec.tiles

    return bytes(out)


def decode_levels(data: bytes) -> List[LevelRecord]:
    if len(data) < HEADER_SIZE:
        raise LevelsFormatError("levels.bin too small for header")

    magic, version, rows, cols, level_count, record_size = HEADER_STRUCT.unpack_from(data, 0)
    if magic != MAGIC:
        raise LevelsFormatError("invalid levels.bin magic")
    if version != VERSION:
        raise LevelsFormatError(f"unsupported levels.bin version: {version}")
    if rows != MAP_ROWS or cols != MAP_COLS:
        raise LevelsFormatError(f"unexpected level dimensions: {rows}x{cols}")
    if level_count < 1:
        raise LevelsFormatError("levels.bin must contain at least one level")
    if record_size != RECORD_SIZE:
        raise LevelsFormatError(f"unexpected record size: {record_size}")

    expected_size = HEADER_SIZE + level_count * record_size
    if len(data) != expected_size:
        raise LevelsFormatError(
            f"levels.bin size mismatch: expected {expected_size} bytes, got {len(data)}"
        )

    levels: List[LevelRecord] = []
    offset = HEADER_SIZE
    for _ in range(level_count):
        record = data[offset : offset + record_size]
        offset += record_size

        raw_name = record[:NAME_LEN]
        nul = raw_name.find(b"\x00")
        if nul <= 0:
            raise LevelsFormatError("invalid level name (missing or empty NUL terminator)")
        name_bytes = raw_name[:nul]
        try:
            name = name_bytes.decode("ascii")
        except UnicodeDecodeError as exc:
            raise LevelsFormatError("level name must be ASCII") from exc
        if any(ch < 0x20 or ch > 0x7E for ch in name_bytes):
            raise LevelsFormatError("level name contains non-printable ASCII")

        wall_tint_argb = struct.unpack_from("<I", record, NAME_LEN)[0]
        tiles = bytearray(record[NAME_LEN + 4 : NAME_LEN + 4 + MAP_TILE_COUNT])

        levels.append(
            LevelRecord(
                name=name,
                wall_tint_argb=wall_tint_argb,
                tiles=tiles,
            )
        )

    return validate_levels(levels)


def read_levels_file(path: Path | str) -> List[LevelRecord]:
    return decode_levels(Path(path).read_bytes())


def write_levels_file(path: Path | str, records: Iterable[LevelRecord]) -> None:
    payload = encode_levels(records)
    Path(path).write_bytes(payload)
