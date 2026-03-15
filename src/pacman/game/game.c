#include "pacman/game/game.h"

#include "pacman/assets/assets.h"
#include "pacman/assets/levels.h"
#include "pacman/game/renderer.h"
#include "engineos/include/timer.h"
#include "stdlib/string.h"

#define POWER_MS       10000u
#define RESET_DELAY_MS 1200u
#define END_SCREEN_MS  2200u

static void consume_item(struct game *self, vec_t tile)
{
    enum item_kind k = items_consume(&self->items, tile);
    if (k == ITEM_DOT) {
        self->score += 10;
    } else if (k == ITEM_POWER) {
        self->score += 50;
        self->power_ms = POWER_MS;
        self->ghost_chain = 0;
        ghosts_set_mode(&self->ghosts, GHOST_NORMAL, GHOST_FRIGHTENED);
    }
}

static void reset_round(struct game *self)
{
    pacman_init(&self->pacman, self->level);
    ghosts_reset(&self->ghosts, &self->board);
    self->power_ms = 0;
    self->ghost_chain = 0;
    consume_item(self, self->pacman.tile);
}

static void kill_pacman(struct game *self)
{
    if (self->power_ms > 0)
        ghosts_set_mode(&self->ghosts, GHOST_FRIGHTENED, GHOST_NORMAL);
    self->power_ms = 0;
    self->ghost_chain = 0;

    if (--self->lives <= 0) {
        self->state = STATE_GAME_OVER;
        self->state_ms = END_SCREEN_MS;
    } else {
        self->state = STATE_ROUND_RESET;
        self->state_ms = RESET_DELAY_MS;
    }
}

static void handle_collisions(struct game *self)
{
    int collisions = ghosts_collide_pacman(&self->ghosts, self->pacman.pos);
    if (collisions < 0) {
        kill_pacman(self);
        return;
    }

    for (int i = 0; i < collisions; i++) {
        self->score += (self->ghost_chain >= 3) ? 1600u : (200u << self->ghost_chain);
        self->ghost_chain++;
    }
}

static void update_running(struct game *self, uint32_t dt)
{
    if (self->power_ms) {
        tick_down(&self->power_ms, dt);
        if (self->power_ms == 0)
            ghosts_set_mode(&self->ghosts, GHOST_FRIGHTENED, GHOST_NORMAL);
    }

    pacman_step(&self->pacman, &self->board, dt);
    consume_item(self, self->pacman.tile);

    if (items_remaining(&self->items) == 0) {
        self->state = STATE_WIN;
        self->state_ms = END_SCREEN_MS;
        return;
    }

    ghosts_step(&self->ghosts, &self->board, dt, self->power_ms > 0);
    handle_collisions(self);
}

void game_init(struct game *self, int level)
{
    memset(self, 0, sizeof(*self));
    self->level = level;
    self->board_origin = (vec_t){
        .x = (FB_WIDTH - LEVEL_COLS * LEVEL_TILE) / 2,
        .y = LEVEL_TILE,
    };

    board_init(&self->board, level);
    items_init(&self->items, level);
    ghosts_init(&self->ghosts, &self->board,
                0xC0DEF00Du ^ (uint32_t)level ^ timer_get_ticks(), level);

    renderer_init();

    self->lives = 3;
    self->state = STATE_READY;
    reset_round(self);
}

void game_tick(struct game *self, uint32_t dt_ms)
{
    switch (self->state) {
        case STATE_READY:
            break;
        case STATE_RUNNING:
            update_running(self, dt_ms);
            break;
        case STATE_ROUND_RESET:
            tick_down(&self->state_ms, dt_ms);
            if (self->state_ms == 0) {
                reset_round(self);
                self->state = STATE_RUNNING;
            }
            break;
        case STATE_WIN:
        case STATE_GAME_OVER:
            tick_down(&self->state_ms, dt_ms);
            if (self->state_ms == 0) self->state = STATE_QUIT;
            break;
        case STATE_QUIT:
            break;
    }

    self->frame++;
}

void game_draw(const struct game *self, uint32_t fps)
{
    renderer_draw(self, fps);
}

void game_on_event(struct game *self, const struct event *event)
{
    if (event->type != ETYPE_KEY || event->key.type != KEYEVENT_TYPE_PRESS) return;

    if (self->state == STATE_READY)
        self->state = STATE_RUNNING;

    switch (event->key.keycode) {
        case KEYCODE_ESC:
            self->state = STATE_QUIT;
            break;
        case KEYCODE_UP:
            pacman_queue_dir(&self->pacman, DIR_UP);
            break;
        case KEYCODE_DOWN:
            pacman_queue_dir(&self->pacman, DIR_DOWN);
            break;
        case KEYCODE_LEFT:
            pacman_queue_dir(&self->pacman, DIR_LEFT);
            break;
        case KEYCODE_RIGHT:
            pacman_queue_dir(&self->pacman, DIR_RIGHT);
            break;
        default:
            break;
    }
}
