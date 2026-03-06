#include "pacman/assets/assets.h"
#include "engineos/include/log.h"
#include "engineos/include/ramfs.h"

struct assets assets;

static struct image load_png(const char *name)
{
    struct ramfs_binary bin = ramfs_get_binary(name);
    if (!ramfs_validate_binary(&bin))
        PANIC("invalid asset: %s", name);
    return fb_decode_png(bin.base, bin.size);
}

static const struct levels *load_levels(const char *name)
{
    struct ramfs_binary bin = ramfs_get_binary(name);
    if (!ramfs_validate_binary(&bin))
        PANIC("invalid asset: %s", name);
    return (const struct levels *)bin.base;
}

static struct font load_font(const char *name, bool set_as_active)
{
    struct font font = {
        .atlas = load_png(name),
        .char_offset = 0,
    };

    font.glyph_w = font.atlas.width / 16;
    font.glyph_h = font.atlas.height / 16;

    if (set_as_active)
        fb_set_font(&font);

    return font;
}

void assets_init(void)
{
    assets = (struct assets){
        .font   = load_font("font.png", true),
        .splash = load_png("splash.png"),
        .tiles  = load_png("tiles.png"),
        .pacman = load_png("pacman.png"),
        .ghosts = load_png("ghosts.png"),
        .levels = load_levels("levels.bin"),
    };
}
