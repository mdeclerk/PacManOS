#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "mb2.h"
#include "heap.h"
#include "ramfs.h"
#include "timer.h"
#include "keyb.h"
#include "fb.h"
#include "irq.h"

#include "engineos/engine/game.h"

void init(void *mb2_ptr)
{
    serial_init();

    mb2_init(mb2_ptr);
    gdt_init();
    idt_init();
    
    heap_init();
    heap_alloc_mb2_modules();

    ramfs_init();
    
    pic_init();
    timer_init(16);
    keyb_init();
    fb_init();

    irq_enable();
    
    game_run();
}
