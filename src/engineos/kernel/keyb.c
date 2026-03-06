#include "keyb.h"
#include "engineos/engine/event.h"
#include "pic.h"
#include "portio.h"
#include "log.h"

#define KEYB_DATA_PORT 0x60

static const char scancode_to_ascii[128] = {
    0,  0,  '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']', 0,
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' '
};

static void keyb_handler(uint8_t irq)
{
    (void)irq;

    uint8_t scancode = inb(KEYB_DATA_PORT);
    bool release = (scancode & 0x80) != 0;
    uint8_t keycode = scancode & 0x7F;

    struct event e = {
        .type = ETYPE_KEY,
        .key.type = release ? KEYEVENT_TYPE_RELEASE : KEYEVENT_TYPE_PRESS,
        .key.scancode = scancode,
        .key.ascii = release ? 0 : scancode_to_ascii[keycode],
        .key.keycode = keycode,
    };

    event_enqueue(&e);
}

void keyb_init(void)
{
    pic_set_handler(PIC_IRQ_KEYBOARD, keyb_handler);
    log("%s: IRQ %d", __func__, PIC_IRQ_KEYBOARD);
}
