#pragma once

#include "pacman/game/game.h"
#include "stdlib/stdint.h"

void renderer_init(void);
void renderer_draw(const struct game *game, uint32_t fps);
