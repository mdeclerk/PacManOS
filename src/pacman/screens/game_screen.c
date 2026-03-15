#include "pacman/screens/screens.h"
#include "pacman/game/game.h"

static struct game game;

static void on_event(const struct event *event)
{
    game_on_event(&game, event);
}

static void tick(uint32_t dt)
{
    game_tick(&game, dt);
    if (game.state == STATE_QUIT)
        screen_set_active(&menu_screen);
}

static void draw(uint32_t fps)
{
    game_draw(&game, fps);
}

void game_screen_start(int level)
{
    game_init(&game, level);
    screen_set_active(&game_screen);
}

struct screen game_screen = {
    .on_event = on_event,
    .tick = tick,
    .draw = draw,
};
