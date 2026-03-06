#pragma once

#include "engineos/include/event.h"
#include "stdlib/stdint.h"

struct screen {
    void (*start)(void);
    void (*stop)(void);
    void (*on_event)(const struct event *event);
    void (*tick)(uint32_t dt_ms);
    void (*draw)(uint32_t fps);
};

struct screen *screen_get_active(void);
void screen_set_active(struct screen *s);
