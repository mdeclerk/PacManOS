#include "pacman/screens/screens.h"
#include "pacman/game/game.h"

static void tick(uint32_t dt)
{
    game_tick(dt);
    if (game_should_quit())
        screen_set_active(&menu_screen);
}

struct screen game_screen = {
    .on_event = game_handle_event,
    .tick = tick,
    .draw = game_draw,
};
