#pragma once

#include "stdlib/stdint.h"

typedef void (*idt_handler_t)(uint8_t vector);

void idt_init(void);
void idt_set_handler(uint8_t vector, idt_handler_t handler);
