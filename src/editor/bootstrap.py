from __future__ import annotations

from .codec import (
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
    write_levels_file,
)
from .paths import LEVELS_PATH

_CLASSIC_BLUE_TINT_ARGB = 0xFF3C8DFF
_ADVANCED_ONE_TINT_ARGB = 0xFF4BC6A1
_ADVANCED_TWO_TINT_ARGB = 0xFFFFA24D

# Human-readable level encoding:
# - '0'..'f': forced wall mask (added to TILE_WALL_FORCED_BASE)
# - '.': dot
# - 'o': power pellet
# - '=': ghost gate
# - '_': empty
# - 'P': pacman spawn
# - 'G': ghost spawn
_DEFAULT_LEVELS = [
    (
        "Classic",
        _CLASSIC_BLUE_TINT_ARGB,
        [
            "5333333333333653333333333336",
            "c............cc............c",
            "c.5336.53336.cc.53336.5336.c",
            "coc__c.c___c.cc.c___c.c__coc",
            "c.933a.9333a.9a.9333a.933a.c",
            "c..........................c",
            "c.5336.56.53333336.56.5336.c",
            "c.933a.cc.9336533a.cc.933a.c",
            "c......cc....cc....cc......c",
            "933336.c9336_cc_533ac.53333a",
            "_____c.c533a_9a_9336c.c_____",
            "_____c.cc__________cc.c_____",
            "_____c.cc_532==136_cc.c_____",
            "13333a.9a_c______c_9a.933332",
            "______.___c__G___c___.______",
            "133336.56_c______c_56.533332",
            "_____c.cc_9333333a_cc.c_____",
            "_____c.cc__________cc.c_____",
            "_____c.cc_53333336_cc.c_____",
            "53333a.9a_9336533a_9a.933336",
            "c............cc............c",
            "c.1376.13332.9a.13332.5732.c",
            "co..cc.......P_.......cc..oc",
            "936.cc.56.53333336.56.cc.53a",
            "53a.9a.cc.9336533a.cc.9a.936",
            "c......cc....cc....cc......c",
            "c.53333a9336.cc.533a933336.c",
            "c.933333333a.9a.933333333a.c",
            "c..........................c",
            "933333333333333333333333333a",
            "____________________________",
        ],
    ),
    (
        "Advanced 1",
        _ADVANCED_ONE_TINT_ARGB,
        [
            "____53336_4_4__4_4_53336____",
            "____c...c_c_c__c_c_c...c____",
            "5333a.4.93a_c__c_93a.4.93336",
            "c.....c.....c__c.....c.....c",
            "c.536.93336.c__c.5333a.536.c",
            "c.coc.....c.c__c.c.....coc.c",
            "c.c.8.532.c.c__c.c.136.8.c.c",
            "c.c...c...c.c__c.c...c...c.c",
            "c.9333a.53e.c__c.d36.9333a.c",
            "c._.....c.c.c__c.c.c....._.c",
            "d3337333a.8.933a.8.93337333e",
            "c...c....__________....c...c",
            "c.13a.136_532==136_532.932.c",
            "c.......c_c______c_c.......c",
            "93333333a_c__G___c_93333333a",
            "__......._c______c_.......__",
            "533333332_9333333a_133333336",
            "c........__________........c",
            "c.533333336.5336.533333336.c",
            "c.c_______c.c__c.c_______c.c",
            "c.c_533373a.c__c.9373336_c.c",
            "c.c_c...c...c__c...c...c_c.c",
            "c.c_c.4.8.4.933a.4.8.4.c_c.c",
            "c.c_c.c...c..P...c...c.c_c.c",
            "c.93a.9333a.5336.9333a.93a.c",
            "c...........c__c...........c",
            "936.5333336.c__c.5333336.53a",
            "__c.c_____c_c__c_c_____c.c__",
            "__coc_____c_c__c_c_____coc__",
            "__93a_____8_8__8_8_____93a__",
            "____________________________",
        ],
    ),
    (
        "Advanced 2",
        _ADVANCED_TWO_TINT_ARGB,
        [
            "5373333333333333333333333736",
            "coc......................coc",
            "c.c.536.536.5336.536.536.c.c",
            "c.c.c.c.c_c.c__c.c_c.c.c.c.c",
            "c.8.c.c.d3a.c__c.93e.c.c.8.c",
            "c...c.c.c...c__c...c.c.c...c",
            "c.13a.93a.53a__936.93a.932.c",
            "c.........c______c.........c",
            "d36.53332.c______c.13336.53e",
            "c.c.c.....c______c.....c.c.c",
            "c.c.c.136_9333333a_532.c.c.c",
            "c.c.c...c__________c...c.c.c",
            "c.c.932.93732==1373a.13a.c.c",
            "c.c.......c______c.......c.c",
            "8.8.53332.c__G___c.13336.8.8",
            "__..c.....c______c.....c..__",
            "4.4.c.5333b333333b3336.c.4.4",
            "c.c.c.c..............c.c.c.c",
            "c.c.c.8.536.5336.536.8.c.c.c",
            "c.c.c...c_c.c__c.c_c...c.c.c",
            "c.c.c.53a_c.c__c.c_936.c.c.c",
            "c.c.c.c___c.c__c.c___c.c.c.c",
            "c.c.8.c___c.933a.c___c.8.c.c",
            "c.c...c___c..P...c___c...c.c",
            "c.9333e_53a.5336.936_d333a.c",
            "c.....c_c...c__c...c_c.....c",
            "93336.c_c.4.c__c.4.c_c.5333a",
            "____c.93a.c.c__c.c.93a.c____",
            "____c.....coc__coc.....c____",
            "____933333b3a__93b33333a____",
            "____________________________",
        ],
    ),
]


def _decode_tile(ch: str) -> int:
    if ch in "0123456789abcdef":
        return TILE_WALL_FORCED_BASE + int(ch, 16)
    if ch == ".":
        return TILE_DOT
    if ch == "o":
        return TILE_POWER
    if ch == "=":
        return TILE_GHOST_GATE
    if ch == "_":
        return TILE_EMPTY
    if ch == "P":
        return TILE_PACMAN_SPAWN
    if ch == "G":
        return TILE_GHOST_SPAWN
    raise ValueError(f"unsupported tile character: {ch!r}")


def _build_level(name: str, wall_tint_argb: int, rows: list[str]) -> LevelRecord:
    if len(rows) != MAP_ROWS:
        raise ValueError(f"{name} level must contain {MAP_ROWS} rows")

    tiles = bytearray(MAP_ROWS * MAP_COLS)
    pacman_spawns = 0
    ghost_spawns = 0
    for row_idx, row in enumerate(rows):
        if len(row) != MAP_COLS:
            raise ValueError(f"{name} level row {row_idx} must contain {MAP_COLS} columns")
        for col_idx, ch in enumerate(row):
            tile = _decode_tile(ch)
            if tile == TILE_PACMAN_SPAWN:
                pacman_spawns += 1
            elif tile == TILE_GHOST_SPAWN:
                ghost_spawns += 1
            tiles[row_idx * MAP_COLS + col_idx] = tile

    if pacman_spawns != 1:
        raise ValueError(f"{name} level must contain exactly one pacman spawn")
    if ghost_spawns != 1:
        raise ValueError(f"{name} level must contain exactly one ghost spawn")

    return LevelRecord(
        name=name,
        wall_tint_argb=wall_tint_argb,
        tiles=tiles,
    )


def bootstrap_levels() -> None:
    LEVELS_PATH.parent.mkdir(parents=True, exist_ok=True)
    default_levels = [_build_level(name, tint, rows) for name, tint, rows in _DEFAULT_LEVELS]
    write_levels_file(LEVELS_PATH, default_levels)
