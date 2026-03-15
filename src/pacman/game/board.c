#include "pacman/game/board.h"
#include "pacman/assets/assets.h"
#include "stdlib/string.h"

static inline bool tile_walkable(uint8_t t)
{
    return !level_is_wall(t);
}

static inline bool tile_walkable_pacman(uint8_t t)
{
    return !level_is_wall(t) && t != LEVEL_TILE_GHOST_GATE;
}

static bool walkable_role(const struct board *self, vec_t tile, bool pacman_role)
{
    int r = grid_wrap(tile.row, LEVEL_ROWS);
    int c = grid_wrap(tile.col, LEVEL_COLS);
    bool (*walkable_fn)(uint8_t) = pacman_role ? tile_walkable_pacman : tile_walkable;

    if (!walkable_fn(self->tiles[r][c]))
        return false;

    if (r == 0 || r == LEVEL_ROWS - 1) {
        int opposite_r = (r == 0) ? (LEVEL_ROWS - 1) : 0;
        if (!walkable_fn(self->tiles[opposite_r][c]))
            return false;
    }

    if (c == 0 || c == LEVEL_COLS - 1) {
        int opposite_c = (c == 0) ? (LEVEL_COLS - 1) : 0;
        if (!walkable_fn(self->tiles[r][opposite_c]))
            return false;
    }

    return true;
}

void board_init(struct board *self, int level)
{
    const struct levels_entry *entry = &assets.levels->entries[level];
    memcpy(self->tiles, entry->tiles, sizeof(self->tiles));
    self->theme_color = entry->theme_color;
}

bool board_walkable_ghost(const struct board *self, vec_t tile)
{
    return walkable_role(self, tile, false);
}

bool board_walkable_pacman(const struct board *self, vec_t tile)
{
    return walkable_role(self, tile, true);
}
