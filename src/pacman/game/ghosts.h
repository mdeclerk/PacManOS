#pragma once

#include "pacman/game/common.h"
#include "pacman/assets/levels.h"
#include "stdlib/stdbool.h"
#include "stdlib/stdint.h"

#define GHOST_COUNT 4
#define GHOST_SPRITE_SZ 32
#define GHOST_OFFSET ((GHOST_SPRITE_SZ - LEVEL_TILE) / 2)

#define GHOST_STEP_NORMAL     140u
#define GHOST_STEP_FRIGHTENED 165u
#define GHOST_STEP_EYES       90u
#define GHOST_RESPAWN_MS      3000u

typedef enum {
    GHOST_NORMAL, GHOST_FRIGHTENED, GHOST_EYES, GHOST_RESPAWN_WAIT, GHOST_EXITING,
} ghost_mode_t;

void ghosts_init(uint32_t seed, int level);
void ghosts_reset(void);
void ghosts_set_mode(ghost_mode_t from, ghost_mode_t to);
void ghosts_step(uint32_t step_ms, bool power_active);
void ghosts_render(uint32_t power_ms_left, vec_t origin);

/* -1 = lethal collision, 0 = no collision, N>0 = N frightened ghosts eaten */
int ghosts_collide_pacman(vec_t pacman_pos);
