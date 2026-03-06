#include "timer.h"
#include "pic.h"
#include "portio.h"
#include "log.h"

#define PIT_CH0_DATA 0x40
#define PIT_CMD      0x43

#define PIT_FREQ     1193182

static volatile uint32_t ticks;
static uint32_t interval_ms;

static void timer_handler(uint8_t irq)
{
    (void)irq;
    ticks++;
}

void timer_init(uint32_t ms)
{
    interval_ms = ms;
    uint32_t divisor = PIT_FREQ * ms / 1000;
    if (divisor > 0xFFFF)
        divisor = 0xFFFF;

    outb(PIT_CMD, 0x36);
    outb(PIT_CH0_DATA, divisor & 0xFF);
    outb(PIT_CH0_DATA, (divisor >> 8) & 0xFF);

    pic_set_handler(PIC_IRQ_TIMER, timer_handler);
    log("%s: IRQ %d, interval %u ms", __func__, PIC_IRQ_TIMER, ms);
}

uint32_t timer_get_ticks(void)
{
    return ticks;
}

uint32_t timer_get_interval_ms(void)
{
    return interval_ms;
}
