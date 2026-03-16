#include "fb.h"
#include "mb2.h"
#include "log.h"
#include "stdlib/string.h"

#define FB_SIZE (FB_WIDTH * FB_HEIGHT * sizeof(color_t))

static color_t *frontbuffer;
static color_t backbuffer[FB_WIDTH * FB_HEIGHT];

static struct font font;

void fb_init(void)
{
    if (mb2_info.fb.width != FB_WIDTH ||
        mb2_info.fb.height != FB_HEIGHT ||
        mb2_info.fb.pitch != FB_PITCH)
        PANIC("framebuffer format invalid");

    frontbuffer = (color_t *)mb2_info.fb.addr;
    if (!frontbuffer)
        PANIC("framebuffer is null");

    fb_clear(FB_BLACK);
    fb_show();

    log("%s: res %ux%u, base %p", __func__, FB_WIDTH, FB_HEIGHT, frontbuffer);
}

void fb_clear(color_t color)
{
    color_t *p = backbuffer;
    uint32_t n = FB_WIDTH * FB_HEIGHT;

    __asm__ volatile (
        "cld; rep stosl"
        : "+D"(p), "+c"(n)
        : "a"(color)
        : "memory"
    );
}

void fb_show(void)
{
    memcpy(frontbuffer, backbuffer, FB_SIZE);
}

static inline uint32_t div255(uint32_t x)
{
    return ((x + 0x80u) + ((x + 0x80u) >> 8)) >> 8;
}

static inline color_t alpha_blend(color_t src, color_t dst, uint8_t alpha)
{
    if (alpha == 0)
        return dst;

    if (alpha == 255)
        return src;

    uint32_t a = alpha;
    uint32_t inv = 255u - a;

    uint32_t src_rb = src & 0x00FF00FFu;
    uint32_t dst_rb = dst & 0x00FF00FFu;
    uint32_t src_g  = src & 0x0000FF00u;
    uint32_t dst_g  = dst & 0x0000FF00u;

    uint32_t rb = src_rb * a + dst_rb * inv + 0x00800080u;
    rb = ((rb + ((rb >> 8) & 0x00FF00FFu)) >> 8) & 0x00FF00FFu;

    uint32_t g = src_g * a + dst_g * inv + 0x00008000u;
    g = ((g + ((g >> 8) & 0x0000FF00u)) >> 8) & 0x0000FF00u;

    return rb | g;
}

static inline color_t tint_color(color_t pixel, color_t tint)
{
    uint32_t tint_r = (tint >> 16) & 0xFFu;
    uint32_t tint_g = (tint >> 8)  & 0xFFu;
    uint32_t tint_b =  tint        & 0xFFu;

    uint32_t pix_r = (pixel >> 16) & 0xFFu;
    uint32_t pix_g = (pixel >> 8)  & 0xFFu;
    uint32_t pix_b =  pixel        & 0xFFu;

    uint32_t r = div255(pix_r * tint_r);
    uint32_t g = div255(pix_g * tint_g);
    uint32_t b = div255(pix_b * tint_b);

    return (pixel & 0xFF000000u) | (r << 16) | (g << 8) | b;
}

void fb_blit(int x, int y, const struct image *src, bool use_src_alpha, color_t tint)
{
    int src_x = 0;
    int src_y = 0;
    int dst_x = x;
    int dst_y = y;
    int blit_w = src->width;
    int blit_h = src->height;
    bool apply_tint = tint != FB_WHITE;

    if (dst_x < 0) {
        src_x = -dst_x;
        blit_w += dst_x;
        dst_x = 0;
    }
    if (dst_y < 0) {
        src_y = -dst_y;
        blit_h += dst_y;
        dst_y = 0;
    }
    if (dst_x + blit_w > FB_WIDTH)
        blit_w = FB_WIDTH - dst_x;
    if (dst_y + blit_h > FB_HEIGHT)
        blit_h = FB_HEIGHT - dst_y;

    if (blit_w <= 0 || blit_h <= 0)
        return;

    for (int yy = 0; yy < blit_h; yy++) {
        color_t *restrict d = &backbuffer[(dst_y + yy) * FB_WIDTH + dst_x];
        color_t *restrict s = &src->pixels[(src_y + yy) * src->pitch + src_x];

        if (!use_src_alpha && !apply_tint) {
            memcpy(d, s, (size_t)blit_w * sizeof(color_t));
            continue;
        }

        for (int xx = 0; xx < blit_w; xx++) {
            color_t p = apply_tint ? tint_color(s[xx], tint) : s[xx];
            if (!use_src_alpha) {
                d[xx] = p;
                continue;
            }
            uint8_t alpha = (p >> 24) & 0xFFu;
            d[xx] = alpha_blend(p, d[xx], alpha);
        }
    }
}

void fb_set_font(const struct font *f)
{
    font = *f;
}

static inline void fb_putc(int x, int y, int src_x, int src_y, color_t fg, color_t bg)
{
    int gx_start = x < 0 ? -x : 0;
    int gx_end   = x + font.glyph_w > FB_WIDTH  ? FB_WIDTH  - x : font.glyph_w;
    int gy_start = y < 0 ? -y : 0;
    int gy_end   = y + font.glyph_h > FB_HEIGHT ? FB_HEIGHT - y : font.glyph_h;

    if (gx_start >= gx_end || gy_start >= gy_end)
        return;

    for (int gy = gy_start; gy < gy_end; gy++) {
        color_t *restrict row      = &backbuffer[( y + gy) * FB_WIDTH + x];
        const color_t *restrict ar = &font.atlas.pixels[(src_y + gy) * font.atlas.width + src_x];

        for (int gx = gx_start; gx < gx_end; gx++) {
            color_t px = ar[gx];
            uint32_t alpha = (px >> 24) & 0xFF;
            row[gx] = alpha_blend(fg, bg != FB_NONE ? bg : row[gx], alpha);
        }
    }
}

void fb_puts(int x, int y, const char *str, color_t fg, color_t bg)
{
    if (!str || !font.atlas.pixels || font.glyph_w <= 0 || font.glyph_h <= 0 || font.atlas.width <= 0)
        return;

    int cols = font.atlas.width / font.glyph_w;
    if (cols <= 0)
        return;

    while (*str) {
        int c = (int)*str++ - font.char_offset;
        if (c < 0 || c > 255) {
            x += font.glyph_w;
            continue;
        }

        int src_x = (c % cols) * font.glyph_w;
        int src_y = (c / cols) * font.glyph_h;

        fb_putc(x, y, src_x, src_y, fg, bg);
        x += font.glyph_w;
    }
}

void fb_get_text_size(const char *str, int *w, int *h)
{
    if (!str) {
        if (w) 
            *w = 0;
        if (h) 
            *h = 0;
        return;
    }

    size_t len = strnlen(str, 512);

    if (w)
        *w = len * font.glyph_w;
    if (h)
        *h = font.glyph_h;
}
