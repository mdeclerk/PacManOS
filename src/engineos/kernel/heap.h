#pragma once

#include "stdlib/stddef.h"
#include "engineos/include/heap.h"

void heap_init(void);
void heap_alloc_mb2_modules(void);
void *heap_alloc_at(void *ptr, size_t size);
void heap_dump(void);
