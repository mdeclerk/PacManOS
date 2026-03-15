#include "pacman/screens/screens.h"
#include "engineos/include/timer.h"

#define BLINK_MS 500u

static void start(void *arg)
{
    (void)arg;
    assets_init();
}

static void on_event(const struct event *event)
{
    if (event->type == ETYPE_KEY && event->key.type == KEYEVENT_TYPE_PRESS)
        screen_set_active(&menu_screen, nullptr);
}

static void draw(uint32_t fps)
{
    (void)fps;
    fb_clear(FB_BLACK);
    draw_splash();

    uint32_t phase = timer_get_ticks() * timer_get_interval_ms() / BLINK_MS;
    if (!(phase & 1u))
        draw_hcentered(FB_HEIGHT * 3 / 4, "PRESS ANY KEY TO START", FB_YELLOW);
}

struct screen init_screen = {
    .start = start,
    .on_event = on_event,
    .draw = draw,
};

struct screen *game_entry(void)
{
    return &init_screen;
}