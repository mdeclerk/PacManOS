from __future__ import annotations

from .models import (
    MAP_COLS,
    MAP_ROWS,
    LevelRecord,
    TILE_DOT,
    TILE_EMPTY,
    TILE_GHOST_GATE,
    TILE_GHOST_SPAWN,
    TILE_PACMAN_SPAWN,
    TILE_POWER,
    TILE_WALL_FORCED_BASE,
    TILE_WALL_FORCED_MAX,
)

TOOL_DEFS: list[tuple[str, str, str]] = [
    ("1", "Empty", "empty"),
    ("2", "Coin", "coin"),
    ("3", "Power", "power"),
    ("4", "Ghost Gate", "ghost_gate"),
    ("5", "Wall (auto)", "wall_auto"),
    ("6", "Pacman Start", "pacman_start"),
    ("7", "Ghost Start", "ghost_start"),
]


def shortcut_levels() -> dict[str, str]:
    return {key: value for key, _label, value in TOOL_DEFS}


def is_wall_tile(tile: int) -> bool:
    return TILE_WALL_FORCED_BASE <= tile < TILE_WALL_FORCED_MAX


def _auto_wall_mask(rec: LevelRecord, row: int, col: int) -> int:
    def wall_at(r: int, c: int) -> bool:
        if r < 0 or r >= MAP_ROWS or c < 0 or c >= MAP_COLS:
            return False
        return is_wall_tile(rec.tiles[r * MAP_COLS + c])

    up = 1 if wall_at(row - 1, col) else 0
    down = 1 if wall_at(row + 1, col) else 0
    left = 1 if wall_at(row, col - 1) else 0
    right = 1 if wall_at(row, col + 1) else 0
    return (up << 3) | (down << 2) | (left << 1) | right


def _retile_auto_neighbors(rec: LevelRecord, row: int, col: int) -> None:
    for rr, cc in ((row, col), (row - 1, col), (row + 1, col), (row, col - 1), (row, col + 1)):
        if rr < 0 or rr >= MAP_ROWS or cc < 0 or cc >= MAP_COLS:
            continue
        idx = rr * MAP_COLS + cc
        if is_wall_tile(rec.tiles[idx]):
            rec.tiles[idx] = TILE_WALL_FORCED_BASE + _auto_wall_mask(rec, rr, cc)


def apply_tool(rec: LevelRecord, row: int, col: int, tool: str) -> str | None:
    idx = row * MAP_COLS + col
    old_tile = rec.tiles[idx]

    if tool == "coin":
        rec.tiles[idx] = TILE_DOT
    elif tool == "power":
        rec.tiles[idx] = TILE_POWER
    elif tool == "ghost_gate":
        rec.tiles[idx] = TILE_GHOST_GATE
    elif tool == "empty":
        rec.tiles[idx] = TILE_EMPTY
        if is_wall_tile(old_tile):
            _retile_auto_neighbors(rec, row, col)
    elif tool == "wall_auto":
        rec.tiles[idx] = TILE_WALL_FORCED_BASE
        _retile_auto_neighbors(rec, row, col)
    elif tool.startswith("wall_forced_"):
        mask = int(tool.rsplit("_", 1)[-1], 10)
        rec.tiles[idx] = TILE_WALL_FORCED_BASE + mask
    elif tool == "pacman_start":
        if is_wall_tile(rec.tiles[idx]):
            return "Pacman start cannot be set on a wall tile"
        for i, tile in enumerate(rec.tiles):
            if tile == TILE_PACMAN_SPAWN:
                rec.tiles[i] = TILE_EMPTY
        rec.tiles[idx] = TILE_PACMAN_SPAWN
    elif tool == "ghost_start":
        if is_wall_tile(rec.tiles[idx]):
            return "Ghost start cannot be set on a wall tile"
        for i, tile in enumerate(rec.tiles):
            if tile == TILE_GHOST_SPAWN:
                rec.tiles[i] = TILE_EMPTY
        rec.tiles[idx] = TILE_GHOST_SPAWN

    return None
