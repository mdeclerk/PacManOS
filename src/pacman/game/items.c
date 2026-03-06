#include "pacman/game/items.h"
#include "pacman/assets/assets.h"
#include "pacman/assets/levels.h"
#include "stdlib/string.h"

#define DOT_COLOR  FB_RGB(0xFF, 0xB8, 0x97)
#define DOT_SIZE   4
#define POWER_SIZE 12
#define POWER_FLASH_PERIOD 20

static struct items {
    uint8_t cells[LEVEL_ROWS][LEVEL_COLS];
    uint16_t remaining;
} items;

static color_t dot_spr[DOT_SIZE * DOT_SIZE];
static color_t power_spr[POWER_SIZE * POWER_SIZE];

static void init_sprites(void)
{
    for (int i = 0; i < DOT_SIZE * DOT_SIZE; i++)
        dot_spr[i] = DOT_COLOR;

    int r = POWER_SIZE / 2 - 1;
    for (int y = 0; y < POWER_SIZE; y++)
        for (int x = 0; x < POWER_SIZE; x++) {
            int dx = x - POWER_SIZE / 2;
            int dy = y - POWER_SIZE / 2;
            power_spr[y * POWER_SIZE + x] = (dx * dx + dy * dy <= r * r) ? DOT_COLOR : 0;
        }
}

void items_init(int level)
{
    const struct levels_entry *entry = &assets.levels->entries[level];

    memset(&items, 0, sizeof(items));
    init_sprites();

    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < LEVEL_COLS; c++) {
            uint8_t t = entry->tiles[r][c];
            if (t == LEVEL_TILE_DOT || t == LEVEL_TILE_POWER) {
                items.cells[r][c] = (t == LEVEL_TILE_DOT) ? ITEM_DOT : ITEM_POWER;
                items.remaining++;
            }
        }
}

enum item_kind items_consume(vec_t tile)
{
    int r = grid_wrap(tile.row, LEVEL_ROWS);
    int c = grid_wrap(tile.col, LEVEL_COLS);
    enum item_kind kind = (enum item_kind)items.cells[r][c];
    if (kind == ITEM_NONE) return ITEM_NONE;
    items.cells[r][c] = ITEM_NONE;
    items.remaining--;
    return kind;
}

uint16_t items_remaining(void)
{
    return items.remaining;
}

void items_render(uint32_t frame, vec_t origin)
{
    int ox = origin.x;
    int oy = origin.y;

    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < LEVEL_COLS; c++) {
            uint8_t item = items.cells[r][c];
            if (item == ITEM_DOT) {
                struct image fb = { dot_spr, DOT_SIZE, DOT_SIZE, DOT_SIZE };
                fb_blit(ox + c * LEVEL_TILE + (LEVEL_TILE - DOT_SIZE) / 2,
                        oy + r * LEVEL_TILE + (LEVEL_TILE - DOT_SIZE) / 2,
                        &fb, true, FB_WHITE);
            } else if (item == ITEM_POWER) {
                if ((frame / (POWER_FLASH_PERIOD / 2)) % 2) continue;
                struct image fb = { power_spr, POWER_SIZE, POWER_SIZE, POWER_SIZE };
                fb_blit(ox + c * LEVEL_TILE + (LEVEL_TILE - POWER_SIZE) / 2,
                        oy + r * LEVEL_TILE + (LEVEL_TILE - POWER_SIZE) / 2,
                        &fb, true, FB_WHITE);
            }
        }
}
