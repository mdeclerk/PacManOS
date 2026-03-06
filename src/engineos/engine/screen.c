#include "screen.h"

static struct screen *active_screen;

struct screen *screen_get_active(void)
{
    return active_screen;
}

void screen_set_active(struct screen *s)
{
    if (active_screen->stop)
        active_screen->stop();

    active_screen = s;

    if (active_screen->start)
        active_screen->start();
}

void screen_init(struct screen *s)
{
    active_screen = s;

    if (active_screen->start)
        active_screen->start();
}

