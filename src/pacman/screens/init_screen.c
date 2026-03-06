#include "pacman/screens/screens.h"
#include "pacman/assets/assets.h"
#include "engineos/include/timer.h"

#define BLINK_MS 500u

static void start(void)
{
    assets_init();
}

static void on_event(const struct event *event)
{
    if (event->type == ETYPE_KEY && event->key.type == KEYEVENT_TYPE_PRESS)
        screen_set_active(&menu_screen);
}

static void draw(uint32_t fps)
{
    (void)fps;
    fb_clear(FB_BLACK);

    struct image splash = assets.splash;
    fb_blit((FB_WIDTH - splash.width) / 2,
            (FB_HEIGHT - splash.height) / 3,
            &splash, true, FB_WHITE);

    uint32_t phase = timer_get_ticks() * timer_get_interval_ms() / BLINK_MS;
    if (!(phase & 1u)) {
        int w, h;
        fb_get_text_size("PRESS ANY KEY TO START", &w, &h);
        fb_puts((FB_WIDTH - w) / 2, FB_HEIGHT * 3 / 4,
                "PRESS ANY KEY TO START", FB_YELLOW, FB_NONE);
    }
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