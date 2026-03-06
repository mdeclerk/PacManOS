#include "serial.h"
#include "portio.h"
#include "log.h"

enum {
    COM1_BASE   = 0x3F8,
    COM_DATA    = COM1_BASE + 0,
    COM_IER     = COM1_BASE + 1,
    COM_FCR     = COM1_BASE + 2,
    COM_LCR     = COM1_BASE + 3,
    COM_MCR     = COM1_BASE + 4,
    COM_LSR     = COM1_BASE + 5
};

static void serial_putc(char c) 
{
    while ((inb(COM_LSR) & 0x20) == 0) {
        /* wait for transmitter to be ready */
    }
    outb(COM_DATA, (unsigned char)c);
}

void serial_puts(const char *str) 
{
    while (*str)
        serial_putc(*str++);
}

void serial_init(void) 
{
    outb(COM_IER,  0x00);
    outb(COM_LCR,  0x80);
    outb(COM_DATA, 0x03);
    outb(COM_IER,  0x00);
    outb(COM_LCR,  0x03);
    outb(COM_FCR,  0xC7);
    outb(COM_MCR,  0x0B);

    log("%s", __func__);
}
