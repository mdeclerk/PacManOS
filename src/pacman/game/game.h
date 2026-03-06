#pragma once

#include "engineos/include/event.h"
#include "stdlib/stdbool.h"
#include "stdlib/stdint.h"

void game_start(int level);
void game_handle_event(const struct event *event);
void game_tick(uint32_t dt_ms);
void game_draw(uint32_t fps);
bool game_should_quit(void);
