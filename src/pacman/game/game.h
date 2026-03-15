#pragma once

#include "pacman/game/board.h"
#include "pacman/game/items.h"
#include "pacman/game/pacman.h"
#include "pacman/game/ghosts.h"
#include "engineos/include/event.h"
#include "stdlib/stdbool.h"
#include "stdlib/stdint.h"

enum game_state {
    STATE_READY, 
    STATE_RUNNING, 
    STATE_ROUND_RESET,
    STATE_WIN, 
    STATE_GAME_OVER, 
    STATE_QUIT,
};

struct game {
    enum game_state state;
    int level;
    int lives;
    vec_t board_origin;
    uint32_t score;
    uint32_t power_ms;
    uint32_t state_ms;
    uint32_t frame;
    uint8_t ghost_chain;
    struct board board;
    struct pacman pacman;
    struct ghosts ghosts;
    struct items items;
};

void game_init(struct game *self, int level);
void game_on_event(struct game *self, const struct event *event);
void game_tick(struct game *self, uint32_t dt_ms);
void game_draw(const struct game *self, uint32_t fps);
