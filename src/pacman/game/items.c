#include "pacman/game/items.h"
#include "pacman/assets/assets.h"
#include "stdlib/string.h"

void items_init(struct items *self, int level)
{
    const struct levels_entry *entry = &assets.levels->entries[level];

    memset(self, 0, sizeof(*self));

    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < LEVEL_COLS; c++) {
            uint8_t t = entry->tiles[r][c];
            if (t == LEVEL_TILE_DOT || t == LEVEL_TILE_POWER) {
                self->cells[r][c] = (t == LEVEL_TILE_DOT) ? ITEM_DOT : ITEM_POWER;
                self->remaining++;
            }
        }
}

enum item_kind items_consume(struct items *self, vec_t tile)
{
    int r = grid_wrap(tile.row, LEVEL_ROWS);
    int c = grid_wrap(tile.col, LEVEL_COLS);
    enum item_kind kind = (enum item_kind)self->cells[r][c];
    if (kind == ITEM_NONE) return ITEM_NONE;
    self->cells[r][c] = ITEM_NONE;
    self->remaining--;
    return kind;
}

uint16_t items_remaining(const struct items *self)
{
    return self->remaining;
}
