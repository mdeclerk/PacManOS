#include "pacman/game/board.h"
#include "pacman/assets/assets.h"
#include "pacman/assets/levels.h"
#include "stdlib/string.h"

#define TILESET_COLS 8
#define TILESET_TILE 24
#define GATE_COLOR   FB_RGB(0xFF, 0xB8, 0xFF)
#define GATE_HEIGHT  4

static struct board {
    color_t theme_color;
    uint8_t tiles[LEVEL_ROWS][LEVEL_COLS];
} board;

static struct image tileset;
static color_t gate_spr[LEVEL_TILE * GATE_HEIGHT];

static inline bool is_wall(uint8_t t)
{
    return (uint8_t)(t - LEVEL_TILE_WALL_BASE) < LEVEL_TILE_WALL_COUNT;
}

static inline bool tile_walkable(uint8_t t)
{
    return !is_wall(t);
}

static inline bool tile_walkable_pacman(uint8_t t)
{
    return !is_wall(t) && t != LEVEL_TILE_GHOST_GATE;
}

static bool board_walkable_role(vec_t tile, bool pacman_role)
{
    int r = grid_wrap(tile.row, LEVEL_ROWS);
    int c = grid_wrap(tile.col, LEVEL_COLS);
    bool (*walkable_fn)(uint8_t) = pacman_role ? tile_walkable_pacman : tile_walkable;

    if (!walkable_fn(board.tiles[r][c]))
        return false;

    /* Edge tiles are only walkable when the opposite edge is also open.
       This keeps intended wrap portals while blocking stray border escapes. */
    if (r == 0 || r == LEVEL_ROWS - 1) {
        int opposite_r = (r == 0) ? (LEVEL_ROWS - 1) : 0;
        if (!walkable_fn(board.tiles[opposite_r][c]))
            return false;
    }

    if (c == 0 || c == LEVEL_COLS - 1) {
        int opposite_c = (c == 0) ? (LEVEL_COLS - 1) : 0;
        if (!walkable_fn(board.tiles[r][opposite_c]))
            return false;
    }

    return true;
}

void board_init(int level)
{
    const struct levels_entry *entry = &assets.levels->entries[level];

    tileset = assets.tiles;
    memcpy(board.tiles, entry->tiles, sizeof(board.tiles));
    board.theme_color = entry->theme_color;

    for (int i = 0; i < LEVEL_TILE * GATE_HEIGHT; i++)
        gate_spr[i] = GATE_COLOR;
}

bool board_walkable_ghost(vec_t tile)
{
    return board_walkable_role(tile, false);
}

bool board_walkable_pacman(vec_t tile)
{
    return board_walkable_role(tile, true);
}

void board_render(vec_t origin)
{
    int ox = origin.x;
    int oy = origin.y;

    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < LEVEL_COLS; c++) {
            uint8_t t = board.tiles[r][c];
            if (is_wall(t)) {
                int shape = t - LEVEL_TILE_WALL_BASE;
                int sx = (shape % TILESET_COLS) * TILESET_TILE;
                int sy = (shape / TILESET_COLS) * TILESET_TILE;
                struct image fb = fb_subrect(&tileset, sx, sy, TILESET_TILE, TILESET_TILE);
                fb_blit(ox + c * LEVEL_TILE, oy + r * LEVEL_TILE, &fb, true, board.theme_color);
            } else if (t == LEVEL_TILE_GHOST_GATE) {
                struct image fb = { gate_spr, LEVEL_TILE, LEVEL_TILE, GATE_HEIGHT };
                fb_blit(ox + c * LEVEL_TILE,
                        oy + r * LEVEL_TILE + (LEVEL_TILE - GATE_HEIGHT) / 2,
                        &fb, true, FB_WHITE);
            }
        }
}
