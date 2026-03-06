#pragma once

#include "pacman/game/common.h"
#include "stdlib/stdint.h"

enum item_kind { ITEM_NONE, ITEM_DOT, ITEM_POWER };

void           items_init(int level);
enum item_kind items_consume(vec_t tile);
uint16_t       items_remaining(void);
void           items_render(uint32_t frame, vec_t origin);
