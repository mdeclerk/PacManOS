#include "pacman/screens/screens.h"
#include "pacman/game/game.h"
#include "stdlib/stdint.h"

static struct game game;

static void start(void *arg)
{
    int level = (int)(intptr_t)arg;
    game_init(&game, level);
}

static void on_event(const struct event *event)
{
    game_on_event(&game, event);
}

static void tick(uint32_t dt)
{
    game_tick(&game, dt);
    if (game.state == STATE_QUIT)
        screen_set_active(&menu_screen, NULL);
}

static void draw(uint32_t fps)
{
    game_draw(&game, fps);
}

struct screen game_screen = {
    .start = start,
    .on_event = on_event,
    .tick = tick,
    .draw = draw,
};
