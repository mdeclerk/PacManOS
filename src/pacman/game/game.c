#include "pacman/game/game.h"

#include "pacman/assets/assets.h"
#include "pacman/game/ghosts.h"
#include "pacman/game/items.h"
#include "pacman/assets/levels.h"
#include "pacman/game/pacman.h"
#include "pacman/game/board.h"
#include "engineos/include/screen.h"
#include "engineos/include/timer.h"
#include "stdlib/stdio.h"
#include "stdlib/string.h"

#define POWER_MS       10000u
#define RESET_DELAY_MS 1200u
#define END_SCREEN_MS  2200u
#define READY_BLINK_MS  200u
#define HUD_PAD          12

enum game_state {
    STATE_READY, STATE_RUNNING, STATE_ROUND_RESET, STATE_WIN, STATE_GAME_OVER, STATE_QUIT,
};

static struct game {
    enum game_state state;
    int level;
    int lives;
    vec_t board_origin;
    uint32_t score;
    uint32_t power_ms;
    uint32_t state_ms;
    uint32_t frame;
    uint8_t ghost_chain;
    bool started;
    bool quit;
} game;

static void tick_down(uint32_t *t, uint32_t dt)
{
    *t = (*t > dt) ? *t - dt : 0;
}

static void draw_centered(const char *text, int cx, int cy, color_t c)
{
    int w, h;
    fb_get_text_size(text, &w, &h);
    fb_puts(cx - w / 2, cy - h / 2, text, c, FB_NONE);
}

static void consume_item(vec_t tile)
{
    enum item_kind k = items_consume(tile);
    if (k == ITEM_DOT) {
        game.score += 10;
    } else if (k == ITEM_POWER) {
        game.score += 50;
        game.power_ms = POWER_MS;
        game.ghost_chain = 0;
        ghosts_set_mode(GHOST_NORMAL, GHOST_FRIGHTENED);
    }
}

static void reset_round(void)
{
    pacman_init(game.level);
    ghosts_reset();
    game.power_ms = 0;
    game.ghost_chain = 0;
    consume_item(pacman_tile());
}

static void kill_pacman(void)
{
    if (game.power_ms > 0)
        ghosts_set_mode(GHOST_FRIGHTENED, GHOST_NORMAL);
    game.power_ms = 0;
    game.ghost_chain = 0;

    if (--game.lives <= 0) {
        game.state = STATE_GAME_OVER;
        game.state_ms = END_SCREEN_MS;
    } else {
        game.state = STATE_ROUND_RESET;
        game.state_ms = RESET_DELAY_MS;
    }
}

static void handle_collisions(void)
{
    int collisions = ghosts_collide_pacman(pacman_pos());
    if (collisions < 0) {
        kill_pacman();
        return;
    }

    for (int i = 0; i < collisions; i++) {
        game.score += (game.ghost_chain >= 3) ? 1600u : (200u << game.ghost_chain);
        game.ghost_chain++;
    }
}

static void update_running(uint32_t dt)
{
    if (game.power_ms) {
        if (game.power_ms > dt)
            game.power_ms -= dt;
        else {
            game.power_ms = 0;
            ghosts_set_mode(GHOST_FRIGHTENED, GHOST_NORMAL);
        }
    }

    pacman_step(dt);
    consume_item(pacman_tile());

    if (items_remaining() == 0) {
        game.state = STATE_WIN;
        game.state_ms = END_SCREEN_MS;
        return;
    }

    ghosts_step(dt, game.power_ms > 0);
    handle_collisions();
}

void game_start(int level)
{
    memset(&game, 0, sizeof(game));
    game.level = level;
    game.board_origin = (vec_t){
        .x = (FB_WIDTH - LEVEL_COLS * LEVEL_TILE) / 2,
        .y = LEVEL_TILE,
    };

    board_init(level);
    items_init(level);
    ghosts_init(0xC0DEF00Du ^ (uint32_t)level ^ timer_get_ticks(), level);

    game.lives = 3;
    game.state = STATE_READY;
    reset_round();
}

void game_tick(uint32_t dt_ms)
{
    if (game.quit) game.state = STATE_QUIT;

    switch (game.state) {
        case STATE_READY:
            if (!game.started) break;
            game.state = STATE_RUNNING;
            /* fall through */
        case STATE_RUNNING:
            update_running(dt_ms);
            break;
        case STATE_ROUND_RESET:
            tick_down(&game.state_ms, dt_ms);
            if (game.state_ms == 0) {
                reset_round();
                game.state = STATE_RUNNING;
            }
            break;
        case STATE_WIN:
        case STATE_GAME_OVER:
            tick_down(&game.state_ms, dt_ms);
            if (game.state_ms == 0) game.state = STATE_QUIT;
            break;
        case STATE_QUIT:
            break;
    }

    game.frame++;
    game.started = false;
    game.quit = false;
}

void game_draw(uint32_t fps)
{
    vec_t bo = game.board_origin;
    vec_t bs = LEVEL_PIXEL_DIMS;

    fb_clear(FB_BLACK);
    board_render(bo);
    items_render(game.frame, bo);
    pacman_render(bo);
    ghosts_render(game.power_ms, bo);

    int hx = bo.x + bs.x + 24;
    int hy = bo.y + HUD_PAD;
    char buf[64];

    fb_puts(hx, hy, "SCORE", FB_WHITE, FB_NONE);
    hy += 18;
    snprintf(buf, sizeof(buf), "%u", game.score);
    fb_puts(hx, hy, buf, FB_YELLOW, FB_NONE);
    hy += 30;

    fb_puts(hx, hy, "LIVES", FB_WHITE, FB_NONE);
    hy += 18;
    snprintf(buf, sizeof(buf), "%d", game.lives);
    fb_puts(hx, hy, buf, FB_GREEN, FB_NONE);

    int cx = bo.x + bs.x / 2;
    int cy = bo.y + bs.y / 2;
    switch (game.state) {
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

void game_handle_event(const struct event *event)
{
    if (event->type != ETYPE_KEY || event->key.type != KEYEVENT_TYPE_PRESS) return;

    game.started = true;
    switch (event->key.keycode) {
        case KEYCODE_ESC:
            game.quit = true;
            break;
        case KEYCODE_UP:
            pacman_queue_dir(DIR_UP);
            break;
        case KEYCODE_DOWN:
            pacman_queue_dir(DIR_DOWN);
            break;
        case KEYCODE_LEFT:
            pacman_queue_dir(DIR_LEFT);
            break;
        case KEYCODE_RIGHT:
            pacman_queue_dir(DIR_RIGHT);
            break;
        default:
            break;
    }
}

bool game_should_quit(void)
{
    return game.state == STATE_QUIT;
}
