#pragma once

#include "pacman/game/common.h"
#include "pacman/assets/levels.h"
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

struct board;

struct ghost {
    vec_t tile;
    vec_t pos;
    dir_t dir;
    ghost_mode_t mode;
    uint32_t rng;
    uint32_t move_accum_ms;
    uint32_t anim_accum_px;
    uint32_t respawn_ms;
    uint8_t anim_phase;
    int id;
};

struct ghost_map {
    vec_t spawn;
    vec_t gate_exit_tile;
    uint8_t gate_cells[LEVEL_ROWS][LEVEL_COLS];
};

struct ghosts {
    struct ghost entries[GHOST_COUNT];
    struct ghost_map map;
};

void ghosts_init(struct ghosts *self, const struct board *board,
                 uint32_t seed, int level);
void ghosts_reset(struct ghosts *self, const struct board *board);
void ghosts_set_mode(struct ghosts *self, ghost_mode_t from, ghost_mode_t to);
void ghosts_step(struct ghosts *self, const struct board *board,
                 uint32_t step_ms, bool power_active);

/* -1 = lethal collision, 0 = no collision, N>0 = N frightened ghosts eaten */
int ghosts_collide_pacman(struct ghosts *self, vec_t pacman_pos);
