#pragma once

#include "pacman/game/common.h"
#include "pacman/assets/levels.h"
#include "stdlib/stdint.h"

#define PACMAN_STEP_MS   110u
#define PACMAN_SPRITE_SZ 32
#define PACMAN_OFFSET    ((PACMAN_SPRITE_SZ - LEVEL_TILE) / 2)

struct board;

struct pacman {
    vec_t tile;
    vec_t pos;
    dir_t dir;
    dir_t queued_dir;
    uint8_t anim_phase;
    uint32_t move_accum_ms;
    uint32_t anim_accum_px;
};

void  pacman_init(struct pacman *self, int level);
void  pacman_queue_dir(struct pacman *self, dir_t dir);
void  pacman_step(struct pacman *self, const struct board *board, uint32_t step_ms);
