#pragma once

#include "pacman/assets/levels.h"
#include "engineos/include/fb.h"

extern struct assets {
    struct font font;
    struct image splash;
    struct image tiles;
    struct image pacman;
    struct image ghosts;
    const struct levels *levels;
} assets;

void assets_init(void);
