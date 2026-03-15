#pragma once

#include "pacman/game/common.h"
#include "pacman/assets/levels.h"
#include "engineos/include/fb.h"

struct board {
    color_t theme_color;
    uint8_t tiles[LEVEL_ROWS][LEVEL_COLS];
};

void board_init(struct board *self, int level);
bool board_walkable_ghost(const struct board *self, vec_t tile);
bool board_walkable_pacman(const struct board *self, vec_t tile);
