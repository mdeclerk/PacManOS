#pragma once

#include "pacman/game/common.h"

void board_init(int level);
bool board_walkable_ghost(vec_t tile);
bool board_walkable_pacman(vec_t tile);
void board_render(vec_t origin);
