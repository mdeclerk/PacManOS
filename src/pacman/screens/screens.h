#pragma once

#include "engineos/include/screen.h"

extern struct screen init_screen;
extern struct screen menu_screen;
extern struct screen game_screen;

void game_screen_start(int level);
