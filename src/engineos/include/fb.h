#pragma once

#include "stdlib/stdint.h"
#include "stdlib/stdbool.h"
#include "stdlib/stddef.h"

typedef uint32_t color_t;

#define FB_WIDTH  1024
#define FB_HEIGHT 768
#define FB_PITCH  (FB_WIDTH * sizeof(color_t))

#define FB_RGB(r, g, b) (0xFF000000u | ((r) << 16) | ((g) << 8) | (b))
#define FB_BLACK   FB_RGB(0x00, 0x00, 0x00)
#define FB_WHITE   FB_RGB(0xFF, 0xFF, 0xFF)
#define FB_RED     FB_RGB(0xFF, 0x00, 0x00)
#define FB_GREEN   FB_RGB(0x00, 0xFF, 0x00)
#define FB_BLUE    FB_RGB(0x00, 0x00, 0xFF)
#define FB_YELLOW  FB_RGB(0xFF, 0xFF, 0x00)
#define FB_NONE    0x00000000u

struct image {
    color_t *pixels;
    int pitch;
    int width;
    int height;
};

struct font {
    struct image atlas;
    int glyph_w;
    int glyph_h;
    uint8_t char_offset;
};

static inline struct image fb_subrect(struct image *im, int x, int y, int w, int h)
{
    return (struct image){
        .pixels = &im->pixels[y * im->pitch + x],
        .pitch  = im->pitch,
        .width  = w,
        .height = h,
    };
}

struct image fb_decode_png(const void *data, size_t size);

void fb_clear(color_t color);

void fb_blit(int x, int y, struct image *im, bool use_src_alpha, color_t tint);

void fb_set_font(const struct font *font);
void fb_puts(int x, int y, const char *str, color_t fg, color_t bg);
void fb_get_text_size(const char *str, int *w, int *h);

