#pragma once

#include "pacman/game/common.h"
#include "pacman/assets/levels.h"
#include "stdlib/stdint.h"

enum item_kind { ITEM_NONE, ITEM_DOT, ITEM_POWER };

struct items {
    uint8_t cells[LEVEL_ROWS][LEVEL_COLS];
    uint16_t remaining;
};

void           items_init(struct items *self, int level);
enum item_kind items_consume(struct items *self, vec_t tile);
uint16_t       items_remaining(const struct items *self);
