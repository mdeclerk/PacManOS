#include "pacman/game/renderer.h"

#include "pacman/assets/assets.h"
#include "pacman/assets/levels.h"
#include "engineos/include/timer.h"
#include "stdlib/stdio.h"

/* ---- board rendering constants ---- */

#define TILESET_COLS 8
#define TILESET_TILE 24
#define GATE_COLOR   FB_RGB(0xFF, 0xB8, 0xFF)
#define GATE_HEIGHT  4

/* ---- item rendering constants ---- */

#define DOT_COLOR  FB_RGB(0xFF, 0xB8, 0x97)
#define DOT_SIZE   4
#define POWER_SIZE 12
#define POWER_FLASH_PERIOD 20

/* ---- ghost rendering constants ---- */

#define BLINK_MS         3000u
#define BLINK_STEP_MS     200u
#define ATLAS_ROW_FRIGHT    4
#define ATLAS_ROW_EYES      5
#define ATLAS_WALK_STRIDE   4

/* ---- HUD constants ---- */

#define READY_BLINK_MS  200u
#define HUD_PAD          12

/* ---- rendering assets (file-scoped) ---- */

static struct image tileset;
static color_t gate_spr[LEVEL_TILE * GATE_HEIGHT];

static color_t dot_spr[DOT_SIZE * DOT_SIZE];
static color_t power_spr[POWER_SIZE * POWER_SIZE];

static struct image ghost_atlas;

/* ---- asset initialization ---- */

static void init_item_sprites(void)
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

static void init_gate_sprite(void)
{
    for (int i = 0; i < LEVEL_TILE * GATE_HEIGHT; i++)
        gate_spr[i] = GATE_COLOR;
}

/* ---- per-object render functions ---- */

static void render_board(const struct board *board, vec_t origin)
{
    int ox = origin.x;
    int oy = origin.y;

    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < LEVEL_COLS; c++) {
            uint8_t t = board->tiles[r][c];
            if (level_is_wall(t)) {
                int shape = t - LEVEL_TILE_WALL_BASE;
                int sx = (shape % TILESET_COLS) * TILESET_TILE;
                int sy = (shape / TILESET_COLS) * TILESET_TILE;
                struct image fb = fb_subrect(&tileset, sx, sy, TILESET_TILE, TILESET_TILE);
                fb_blit(ox + c * LEVEL_TILE, oy + r * LEVEL_TILE,
                        &fb, true, board->theme_color);
            } else if (t == LEVEL_TILE_GHOST_GATE) {
                struct image fb = { gate_spr, LEVEL_TILE, LEVEL_TILE, GATE_HEIGHT };
                fb_blit(ox + c * LEVEL_TILE,
                        oy + r * LEVEL_TILE + (LEVEL_TILE - GATE_HEIGHT) / 2,
                        &fb, true, FB_WHITE);
            }
        }
}

static void render_items(const struct items *items, uint32_t frame, vec_t origin)
{
    int ox = origin.x;
    int oy = origin.y;

    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < LEVEL_COLS; c++) {
            uint8_t item = items->cells[r][c];
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

static void render_pacman(const struct pacman *pac, vec_t origin)
{
    dir_t dir = (pac->dir != DIR_NONE) ? pac->dir : pac->queued_dir;
    int col = pac->anim_phase & 3;
    struct image fb = fb_subrect(&assets.pacman,
                                 col * PACMAN_SPRITE_SZ,
                                 dir_to_row(dir) * PACMAN_SPRITE_SZ,
                                 PACMAN_SPRITE_SZ, PACMAN_SPRITE_SZ);
    fb_blit(origin.x + pac->pos.x - PACMAN_OFFSET,
            origin.y + pac->pos.y - PACMAN_OFFSET, &fb, true, FB_WHITE);
}

static void render_ghost(const struct ghost *g, bool blink_normal, vec_t origin)
{
    bool eyes = g->mode == GHOST_EYES || g->mode == GHOST_RESPAWN_WAIT;
    int row = eyes ? ATLAS_ROW_EYES
            : (g->mode == GHOST_FRIGHTENED && !blink_normal) ? ATLAS_ROW_FRIGHT
            : g->id % GHOST_COUNT;
    static const int dir_col[] = { 2, 3, 0, 1, 1 };
    int col = dir_col[g->dir];
    if (!eyes && (g->anim_phase & 1))
        col += ATLAS_WALK_STRIDE;

    struct image fb = fb_subrect(&ghost_atlas,
                                 col * GHOST_SPRITE_SZ,
                                 row * GHOST_SPRITE_SZ,
                                 GHOST_SPRITE_SZ,
                                 GHOST_SPRITE_SZ);
    fb_blit(origin.x + g->pos.x - GHOST_OFFSET,
            origin.y + g->pos.y - GHOST_OFFSET, &fb, true, FB_WHITE);
}

static void render_ghosts(const struct ghosts *ghosts, uint32_t power_ms, vec_t origin)
{
    bool blink = power_ms > 0 && power_ms <= BLINK_MS
                 && ((power_ms / BLINK_STEP_MS) & 1u);
    for (int i = 0; i < GHOST_COUNT; i++)
        render_ghost(&ghosts->entries[i], blink, origin);
}

static void draw_centered(const char *text, int cx, int cy, color_t c)
{
    int w, h;
    fb_get_text_size(text, &w, &h);
    fb_puts(cx - w / 2, cy - h / 2, text, c, FB_NONE);
}

static void render_hud(const struct game *game, vec_t bo, vec_t bs, uint32_t fps)
{
    int hx = bo.x + bs.x + 24;
    int hy = bo.y + HUD_PAD;
    char buf[64];

    fb_puts(hx, hy, "SCORE", FB_WHITE, FB_NONE);
    hy += 18;
    snprintf(buf, sizeof(buf), "%u", game->score);
    fb_puts(hx, hy, buf, FB_YELLOW, FB_NONE);
    hy += 30;

    fb_puts(hx, hy, "LIVES", FB_WHITE, FB_NONE);
    hy += 18;
    snprintf(buf, sizeof(buf), "%d", game->lives);
    fb_puts(hx, hy, buf, FB_GREEN, FB_NONE);

    int cx = bo.x + bs.x / 2;
    int cy = bo.y + bs.y / 2;
    switch (game->state) {
        case STATE_READY:
            if (!((timer_get_ticks() * timer_get_interval_ms() / READY_BLINK_MS) & 1u))
                draw_centered("READY?", cx, cy, FB_YELLOW);
            break;
        case STATE_ROUND_RESET:
            draw_centered("READY", cx, cy, FB_YELLOW);
            break;
        case STATE_WIN:
            draw_centered("YOU WIN", cx, cy, FB_GREEN);
            break;
        case STATE_GAME_OVER:
            draw_centered("GAME OVER", cx, cy, FB_RED);
            break;
        default:
            break;
    }

    snprintf(buf, sizeof(buf), "FPS:%u", fps);
    fb_puts(4, 2, buf, FB_GREEN, FB_NONE);
}

/* ---- public API ---- */

void renderer_init(void)
{
    tileset = assets.tiles;
    ghost_atlas = assets.ghosts;
    init_gate_sprite();
    init_item_sprites();
}

void renderer_draw(const struct game *game, uint32_t fps)
{
    fb_clear(FB_BLACK);

    vec_t bo = game->board_origin;

    render_board(&game->board, bo);
    render_items(&game->items, game->frame, bo);
    render_pacman(&game->pacman, bo);
    render_ghosts(&game->ghosts, game->power_ms, bo);
    render_hud(game, bo, LEVEL_PIXEL_DIMS, fps);
}
