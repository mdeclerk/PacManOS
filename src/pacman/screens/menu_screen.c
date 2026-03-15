#include "pacman/screens/screens.h"
#include "pacman/assets/assets.h"
#include "stdlib/stdio.h"

static int selected_level;

static void on_event(const struct event *event)
{
    if (event->type != ETYPE_KEY || event->key.type != KEYEVENT_TYPE_PRESS)
        return;

    int count = assets.levels->header.count;
    switch (event->key.keycode) {
        case KEYCODE_UP:
            selected_level = (selected_level + count - 1) % count;
            break;
        case KEYCODE_DOWN:
            selected_level = (selected_level + 1) % count;
            break;
        case KEYCODE_ENTER:
            screen_set_active(&game_screen, (void *)(intptr_t)selected_level);
            break;
        default: break;
    }
}

static void draw_hcentered(int y, const char *text, color_t color)
{
    int w, h;
    fb_get_text_size(text, &w, &h);
    fb_puts((FB_WIDTH - w) / 2, y, text, color, FB_NONE);
}

static void draw(uint32_t fps)
{
    (void)fps;
    fb_clear(FB_BLACK);

    struct image splash = assets.splash;
    fb_blit((FB_WIDTH - splash.width) / 2, (FB_HEIGHT - splash.height) / 3,
            &splash, true, FB_WHITE);

    int count = assets.levels->header.count;
    int tw, th;
    fb_get_text_size("A", &tw, &th);

    int y = FB_HEIGHT * 2 / 3;
    for (int i = 0; i < count; i++) {
        const char *name = assets.levels->entries[i].name;
        if (i == selected_level) {
            char buf[96];
            snprintf(buf, sizeof(buf), "> %s <", name);
            draw_hcentered(y, buf, FB_YELLOW);
        } else {
            draw_hcentered(y, name, FB_WHITE);
        }
        y += th + 8;
    }
}

struct screen menu_screen = {
    .on_event = on_event,
    .draw = draw,
};
