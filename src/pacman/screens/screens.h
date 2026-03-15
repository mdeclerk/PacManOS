#pragma once

#include "engineos/include/screen.h"
#include "pacman/assets/assets.h"

extern struct screen init_screen;
extern struct screen menu_screen;
extern struct screen game_screen;

/* ---- shared drawing helpers ---- */

static inline void draw_splash(void)
{
    struct image splash = assets.splash;
    fb_blit((FB_WIDTH - splash.width) / 2,
            (FB_HEIGHT - splash.height) / 3,
            &splash, true, FB_WHITE);
}

static inline void draw_hcentered(int y, const char *text, color_t color)
{
    int w, h;
    fb_get_text_size(text, &w, &h);
    fb_puts((FB_WIDTH - w) / 2, y, text, color, FB_NONE);
}
