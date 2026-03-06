#pragma once

#include "stdlib/stdint.h"

#define KEYEVENT_TYPE_PRESS     1
#define KEYEVENT_TYPE_RELEASE   2

#define KEYCODE_ESC    0x01
#define KEYCODE_ENTER  0x1C
#define KEYCODE_F10    0x44
#define KEYCODE_UP     0x48
#define KEYCODE_DOWN   0x50
#define KEYCODE_LEFT   0x4B
#define KEYCODE_RIGHT  0x4D

struct key_event {
    uint8_t type;
    uint8_t scancode;
    uint8_t ascii;
    uint8_t keycode;
};
