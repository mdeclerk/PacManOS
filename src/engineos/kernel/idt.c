#include "idt.h"
#include "log.h"
#include "stdlib/stdint.h"

#define IDT_SIZE 256

#define IDT_FLAG_PRESENT 0x80
#define IDT_FLAG_INT_GATE 0x0E
#define IDT_FLAG_RING0 0x00
#define IDT_FLAGS (IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE)

struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern uintptr_t isr_table[IDT_SIZE]; // isr.S

static struct idt_entry idt[IDT_SIZE];
static idt_handler_t handlers[IDT_SIZE];

void idt_set_handler(uint8_t vector, idt_handler_t handler)
{
    handlers[vector] = handler;
}

void isr_dispatch(uint32_t vector, uint32_t err_code)
{
    if (!handlers[vector])
        PANIC("Unhandled interrupt vector %u, err=%x", vector, err_code);
    
    handlers[vector](vector);
}

static void idt_set_gate(uint8_t vector, uintptr_t base, uint8_t flags)
{
    struct idt_entry *e = &idt[vector];
    e->base_lo = base & 0xFFFFu;
    e->sel = 0x08;
    e->always0 = 0;
    e->flags = flags;
    e->base_hi = (base >> 16) & 0xFFFFu;
}

void idt_init(void)
{
    for (int i = 0; i < IDT_SIZE; i++)
        idt_set_gate(i, isr_table[i], IDT_FLAGS);

    struct idt_ptr idt_ptr = {
        .limit = (uint16_t)(sizeof(idt) - 1),
        .base = (uint32_t)(uintptr_t)&idt
    };
    asm volatile("lidt (%0)" : : "r"(&idt_ptr));

    log("%s: base %x, limit %x", __func__, idt_ptr.base, idt_ptr.limit);
}
