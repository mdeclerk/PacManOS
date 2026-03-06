#pragma once

#include "stdlib/stdint.h"
#include "stdlib/stdbool.h"

/* ---- vector / direction primitives ---- */

typedef union {
    struct { int row, col; };
    struct { int x, y; };
} vec_t;

typedef enum {
    DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE,
} dir_t;

static inline int dir_dr(dir_t d) { return (d == DIR_DOWN) - (d == DIR_UP); }
static inline int dir_dc(dir_t d) { return (d == DIR_RIGHT) - (d == DIR_LEFT); }

static inline dir_t dir_opposite(dir_t d)
{
    static const dir_t tbl[] = { DIR_DOWN, DIR_UP, DIR_RIGHT, DIR_LEFT, DIR_NONE };
    return tbl[d];
}

static inline int dir_to_row(dir_t d)
{
    static const int tbl[] = { 2, 3, 1, 0, 0 };
    return tbl[d];
}

/* ---- grid helpers ---- */

static inline int grid_wrap(int v, int limit)
{
    return ((v % limit) + limit) % limit;
}

static inline vec_t grid_step(vec_t tile, dir_t dir, vec_t dims)
{
    tile.row = grid_wrap(tile.row + dir_dr(dir), dims.row);
    tile.col = grid_wrap(tile.col + dir_dc(dir), dims.col);
    return tile;
}

static inline bool grid_aligned_px(vec_t pos, int tile)
{
    return (pos.x % tile) == 0 && (pos.y % tile) == 0;
}

static inline vec_t grid_px_to_tile(vec_t pos, int tile, vec_t dims)
{
    return (vec_t){
        .row = grid_wrap((pos.y + tile / 2) / tile, dims.row),
        .col = grid_wrap((pos.x + tile / 2) / tile, dims.col),
    };
}

static inline void grid_wrap_px(vec_t *pos, vec_t sz)
{
    if (pos->x < 0)       pos->x += sz.x;
    else if (pos->x >= sz.x) pos->x -= sz.x;
    if (pos->y < 0)       pos->y += sz.y;
    else if (pos->y >= sz.y) pos->y -= sz.y;
}

static inline void grid_move_px(vec_t *pos, dir_t dir, vec_t sz)
{
    pos->x += dir_dc(dir);
    pos->y += dir_dr(dir);
    grid_wrap_px(pos, sz);
}

static inline void grid_advance_anim(uint32_t *accum, uint8_t *phase,
                                     uint32_t moved, uint32_t step)
{
    *accum += moved;
    while (*accum >= step) {
        *accum -= step;
        (*phase)++;
    }
}

static inline bool grid_overlap_wrap_px(vec_t a, vec_t b, vec_t sz, int half)
{
    int dx = a.x - b.x, dy = a.y - b.y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dx > sz.x / 2) dx = sz.x - dx;
    if (dy > sz.y / 2) dy = sz.y - dy;
    return dx <= half && dy <= half;
}
