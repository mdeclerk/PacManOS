#include "pic.h"
#include "idt.h"
#include "portio.h"
#include "log.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

#define PIC_EOI   0x20

#define PIC1_VECTOR_OFFSET 32
#define PIC2_VECTOR_OFFSET 40

#define IRQ_COUNT 16

static irq_handler_t irq_handlers[IRQ_COUNT];

static void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

static void pic_irq_dispatch(uint8_t vector)
{
    uint8_t irq = vector - PIC1_VECTOR_OFFSET;
    if (irq_handlers[irq])
        irq_handlers[irq](irq);
    pic_send_eoi(irq);
}

static void pic_set_mask(uint8_t irq)
{
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t bit = (irq < 8) ? irq : irq - 8;
    outb(port, inb(port) | (1 << bit));
}

static void pic_clear_mask(uint8_t irq)
{
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t bit = (irq < 8) ? irq : irq - 8;
    outb(port, inb(port) & ~(1 << bit));
    if (irq >= 8)
        pic_clear_mask(PIC_IRQ_CASCADE);
}

void pic_set_handler(uint8_t irq, irq_handler_t handler)
{
    if (irq >= IRQ_COUNT)
        PANIC("invalid IRQ %u", irq);

    irq_handlers[irq] = handler;

    if (handler)
        pic_clear_mask(irq);
    else
        pic_set_mask(irq);
}

void pic_init(void)
{
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

    outb(PIC1_DATA, PIC1_VECTOR_OFFSET);
    outb(PIC2_DATA, PIC2_VECTOR_OFFSET);

    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    for (int i = 0; i < IRQ_COUNT; i++)
        idt_set_handler(PIC1_VECTOR_OFFSET + i, pic_irq_dispatch);

    log("%s", __func__);
}
