from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LEVELS_PATH = ROOT / "assets" / "pacman" / "levels.bin"
TILESET_PATH = ROOT / "assets" / "pacman" / "tiles.png"
TILESET_TILE = 24
TILESET_COLS = 8
TILE_SIZE = TILESET_TILE
