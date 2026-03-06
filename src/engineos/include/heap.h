#pragma once

#include "stdlib/stddef.h"

void *heap_alloc(size_t size);
void heap_free(void *ptr);