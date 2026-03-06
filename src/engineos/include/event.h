#pragma once

#include "stdlib/stdint.h"
#include "stdlib/stdbool.h"

#include "engineos/include/keyb.h"

enum {
    ETYPE_NONE = 0,
    ETYPE_KEY  = 1,
};

struct event {
    uint8_t type;
    union {
        struct key_event key;
    };
};
