#pragma once

#include "pacman/game/common.h"
#include "pacman/assets/levels.h"
#include "engineos/include/fb.h"

enum pathfinder_flags {
    PF_WALLS_ONLY,       /* only walls block (gates passable) */
    PF_WALLS_AND_GATES,  /* walls and gates block */
};

struct board {
    color_t theme_color;
    uint8_t tiles[LEVEL_ROWS][LEVEL_COLS];
};

void  board_init(struct board *self, int level);
bool  board_walkable(const struct board *self, vec_t tile, enum pathfinder_flags flags);
dir_t board_find_path(const struct board *self, vec_t start, vec_t target,
                      enum pathfinder_flags flags);
