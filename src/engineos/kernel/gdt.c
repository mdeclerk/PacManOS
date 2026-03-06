#include "gdt.h"
#include "log.h"

#include "stdlib/stdint.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  flags_limit_high;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[] = {
    {0}, // Null descriptor
    {0xFFFF, 0, 0, 0x9A, 0xCF, 0}, // Code segment: base=0, limit=0xFFFFF, 4KB granularity, 32-bit, ring 0
    {0xFFFF, 0, 0, 0x92, 0xCF, 0}, // Data segment: base=0, limit=0xFFFFF, 4KB granularity, 32-bit, ring 0
};

static struct gdt_ptr gdt_ptr = {
    .limit = sizeof(gdt) - 1,
    .base = (uint32_t)gdt,
};

extern void gdt_flush(struct gdt_ptr *ptr);

void gdt_init(void)
{
    gdt_flush(&gdt_ptr);
    log("%s: base %x, limit %x", __func__, gdt_ptr.base, gdt_ptr.limit);
}
