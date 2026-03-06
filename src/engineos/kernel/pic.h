#pragma once

#include "stdlib/stdint.h"

#define PIC_IRQ_TIMER    0
#define PIC_IRQ_KEYBOARD 1
#define PIC_IRQ_CASCADE  2
#define PIC_IRQ_COM2     3
#define PIC_IRQ_COM1     4
#define PIC_IRQ_LPT2     5
#define PIC_IRQ_FLOPPY   6
#define PIC_IRQ_LPT1     7
#define PIC_IRQ_RTC      8
#define PIC_IRQ_ATA1     14
#define PIC_IRQ_ATA2     15

typedef void (*irq_handler_t)(uint8_t irq);

void pic_init(void);
void pic_set_handler(uint8_t irq, irq_handler_t handler);
