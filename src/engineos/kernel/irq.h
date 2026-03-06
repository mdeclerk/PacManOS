#pragma once

static inline void irq_enable(void)
{
    asm volatile("sti");
}

static inline void irq_disable(void)
{
    asm volatile("cli");
}
