#pragma once

#include "pacman/game/common.h"
#include "engineos/include/fb.h"
#include "stdlib/stdint.h"

#define LEVEL_ROWS 31
#define LEVEL_COLS 28
#define LEVEL_TILE 24
#define LEVEL_NAME_LEN 32

#define LEVEL_TILE_DIMS  ((vec_t){ .row = LEVEL_ROWS, .col = LEVEL_COLS })
#define LEVEL_PIXEL_DIMS ((vec_t){ .x = LEVEL_COLS * LEVEL_TILE, .y = LEVEL_ROWS * LEVEL_TILE })

#define LEVEL_TILE_EMPTY        0
#define LEVEL_TILE_DOT          5
#define LEVEL_TILE_POWER        6
#define LEVEL_TILE_GHOST_GATE   7
#define LEVEL_TILE_PACMAN_SPAWN 8
#define LEVEL_TILE_GHOST_SPAWN  9
#define LEVEL_TILE_WALL_BASE    0x10
#define LEVEL_TILE_WALL_COUNT   16
#define LEVEL_TILE_WALL_MAX     (LEVEL_TILE_WALL_BASE + LEVEL_TILE_WALL_COUNT)

static inline bool level_is_wall(uint8_t t)
{
    return (uint8_t)(t - LEVEL_TILE_WALL_BASE) < LEVEL_TILE_WALL_COUNT;
}

struct levels_header {
    char magic[4];
    uint16_t version, rows, cols, count, record_size;
} __attribute__((packed));

struct levels_entry {
    char    name[LEVEL_NAME_LEN];
    color_t theme_color;
    uint8_t tiles[LEVEL_ROWS][LEVEL_COLS];
} __attribute__((packed));

struct levels {
    struct levels_header header;
    struct levels_entry entries[];
} __attribute__((packed));
