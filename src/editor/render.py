from __future__ import annotations

import tkinter as tk

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
from .paths import TILESET_COLS, TILESET_TILE, TILE_SIZE

GRID_COLOR = "#2A2A2A"


def argb_to_hex(argb: int) -> str:
    return f"#{(argb >> 16) & 0xFF:02X}{(argb >> 8) & 0xFF:02X}{argb & 0xFF:02X}"


def hex_to_argb(value: str) -> int:
    value = value.strip()
    if value.startswith("#"):
        value = value[1:]
    if len(value) != 6:
        raise ValueError("expected 6 hex digits")
    rgb = int(value, 16)
    return 0xFF000000 | rgb


def init_wall_tile_images(tileset_path: str) -> tuple[tk.PhotoImage, dict[int, tk.PhotoImage]]:
    tileset_image = tk.PhotoImage(file=tileset_path)
    wall_tile_images: dict[int, tk.PhotoImage] = {}
    for shape in range(16):
        src_x = (shape % TILESET_COLS) * TILESET_TILE
        src_y = (shape // TILESET_COLS) * TILESET_TILE
        img = tk.PhotoImage(width=TILESET_TILE, height=TILESET_TILE)
        img.tk.call(
            img,
            "copy",
            tileset_image,
            "-from",
            src_x,
            src_y,
            src_x + TILESET_TILE,
            src_y + TILESET_TILE,
            "-to",
            0,
            0,
        )
        wall_tile_images[shape] = img
    return tileset_image, wall_tile_images


def make_wall_preview(image: tk.PhotoImage) -> tk.PhotoImage:
    return image.subsample(2, 2)


def _parse_rgb(color: str | tuple[int, int, int]) -> tuple[int, int, int]:
    if isinstance(color, tuple):
        return int(color[0]), int(color[1]), int(color[2])
    value = str(color).strip()
    if value.startswith("#") and len(value) >= 7:
        return int(value[1:3], 16), int(value[3:5], 16), int(value[5:7], 16)
    if value.startswith("(") and value.endswith(")"):
        parts = [p.strip() for p in value[1:-1].split(",")]
        if len(parts) >= 3:
            return int(parts[0]), int(parts[1]), int(parts[2])
    return 0, 0, 0


def get_tinted_wall_image(
    shape: int,
    tint_argb: int,
    wall_tile_images: dict[int, tk.PhotoImage],
    tinted_wall_cache: dict[tuple[int, int], tk.PhotoImage],
) -> tk.PhotoImage:
    key = (shape, tint_argb & 0xFFFFFF)
    cached = tinted_wall_cache.get(key)
    if cached is not None:
        return cached

    base = wall_tile_images[shape]
    out = tk.PhotoImage(width=TILESET_TILE, height=TILESET_TILE)
    tr = (tint_argb >> 16) & 0xFF
    tg = (tint_argb >> 8) & 0xFF
    tb = tint_argb & 0xFF

    for y in range(TILESET_TILE):
        row_pixels: list[str] = []
        for x in range(TILESET_TILE):
            r, g, b = _parse_rgb(base.get(x, y))
            if r == 0 and g == 0 and b == 0:
                row_pixels.append("#000000")
            else:
                rr = (r * tr) // 255
                gg = (g * tg) // 255
                bb = (b * tb) // 255
                row_pixels.append(f"#{rr:02X}{gg:02X}{bb:02X}")
        out.put("{" + " ".join(row_pixels) + "}", to=(0, y))

    tinted_wall_cache[key] = out
    return out


def tile_fill(tile: int) -> str:
    _ = tile
    return "#000000"


def render_canvas(
    canvas: tk.Canvas,
    rec: LevelRecord,
    wall_tile_images: dict[int, tk.PhotoImage],
    tinted_wall_cache: dict[tuple[int, int], tk.PhotoImage],
) -> None:
    canvas.delete("all")

    for row in range(MAP_ROWS):
        for col in range(MAP_COLS):
            idx = row * MAP_COLS + col
            tile = rec.tiles[idx]
            x0 = col * TILE_SIZE
            y0 = row * TILE_SIZE
            x1 = x0 + TILE_SIZE
            y1 = y0 + TILE_SIZE

            canvas.create_rectangle(
                x0,
                y0,
                x1,
                y1,
                fill=tile_fill(tile),
                outline="",
            )

            if tile == TILE_DOT:
                cx = x0 + TILE_SIZE // 2
                cy = y0 + TILE_SIZE // 2
                canvas.create_oval(cx - 2, cy - 2, cx + 2, cy + 2, fill="#FFD2A6", outline="")
            elif tile == TILE_POWER:
                cx = x0 + TILE_SIZE // 2
                cy = y0 + TILE_SIZE // 2
                canvas.create_oval(cx - 5, cy - 5, cx + 5, cy + 5, fill="#FFE1C7", outline="")
            elif tile == TILE_GHOST_GATE:
                canvas.create_rectangle(
                    x0 + 2,
                    y0 + TILE_SIZE // 2 - 2,
                    x1 - 2,
                    y0 + TILE_SIZE // 2 + 2,
                    fill="#F08FFF",
                    outline="",
                )
            elif TILE_WALL_FORCED_BASE <= tile < TILE_WALL_FORCED_MAX:
                wall_type = tile - TILE_WALL_FORCED_BASE
                wall_img = get_tinted_wall_image(
                    wall_type,
                    rec.wall_tint_argb,
                    wall_tile_images,
                    tinted_wall_cache,
                )
                canvas.create_image(x0, y0, image=wall_img, anchor="nw")

    board_w = MAP_COLS * TILE_SIZE
    board_h = MAP_ROWS * TILE_SIZE
    for col in range(MAP_COLS + 1):
        x = col * TILE_SIZE
        canvas.create_line(x, 0, x, board_h, fill=GRID_COLOR)
    for row in range(MAP_ROWS + 1):
        y = row * TILE_SIZE
        canvas.create_line(0, y, board_w, y, fill=GRID_COLOR)

    p_idx = -1
    g_idx = -1
    for i, tile in enumerate(rec.tiles):
        if tile == TILE_PACMAN_SPAWN:
            p_idx = i
        elif tile == TILE_GHOST_SPAWN:
            g_idx = i

    if p_idx >= 0:
        px = (p_idx % MAP_COLS) * TILE_SIZE + TILE_SIZE // 2
        py = (p_idx // MAP_COLS) * TILE_SIZE + TILE_SIZE // 2
        canvas.create_oval(px - 7, py - 7, px + 7, py + 7, fill="#F7D54A", outline="#000000", width=1)
        canvas.create_text(px, py, text="P", fill="#000000", font=("TkDefaultFont", 8, "bold"))

    if g_idx >= 0:
        gx = (g_idx % MAP_COLS) * TILE_SIZE + TILE_SIZE // 2
        gy = (g_idx // MAP_COLS) * TILE_SIZE + TILE_SIZE // 2
        canvas.create_oval(gx - 7, gy - 7, gx + 7, gy + 7, fill="#FF5F5F", outline="#000000", width=1)
        canvas.create_text(gx, gy, text="G", fill="#000000", font=("TkDefaultFont", 8, "bold"))
