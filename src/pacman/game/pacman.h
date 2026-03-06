#pragma once

#include "pacman/game/common.h"
#include "pacman/assets/levels.h"
#include "stdlib/stdint.h"

#define PACMAN_STEP_MS   110u
#define PACMAN_SPRITE_SZ 32
#define PACMAN_OFFSET    ((PACMAN_SPRITE_SZ - LEVEL_TILE) / 2)

void  pacman_init(int level);
void  pacman_queue_dir(dir_t dir);
void  pacman_step(uint32_t step_ms);
void  pacman_render(vec_t origin);
vec_t pacman_tile(void);
vec_t pacman_pos(void);
