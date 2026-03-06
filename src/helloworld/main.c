#include "engineos/include/fb.h"
#include "engineos/include/log.h"
#include "engineos/include/ramfs.h"
#include "engineos/include/screen.h"

#define TEXT "Hello, World!"

static struct font  font;
static struct image background;

static struct image load_png(const char *name)
{
    struct ramfs_binary bin = ramfs_get_binary(name);
    if (!ramfs_validate_binary(&bin))
        PANIC("invalid asset: %s", name);
    return fb_decode_png(bin.base, bin.size);
}

static void start(void)
{
    background = load_png("test.png");

    font = (struct font){
        .atlas       = load_png("font.png"),
        .glyph_w     = 32,
        .glyph_h     = 32,
        .char_offset = 0,
    };
    fb_set_font(&font);
}

static void draw(uint32_t fps)
{
    (void)fps;

    fb_blit(0, 0, &background, false, FB_WHITE);

    int tw, th;
    fb_get_text_size(TEXT, &tw, &th);

    int rx = (FB_WIDTH  - tw) / 2;
    int ry = (FB_HEIGHT - th) / 2;

    fb_puts(rx, ry, TEXT, FB_WHITE, FB_BLACK);
}

static struct screen main_screen = {
    .start = start,
    .draw  = draw,
};

struct screen *game_entry(void)
{
    return &main_screen;
}
