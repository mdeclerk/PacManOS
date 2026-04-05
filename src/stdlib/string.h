#pragma once

#include "stddef.h"
#include "stdint.h"

#define memset  __builtin_memset
#define memcpy  __builtin_memcpy
#define memcmp  __builtin_memcmp
#define strncmp __builtin_strncmp
#define strnlen __builtin_strnlen
#define strncpy __builtin_strncpy

bool strneq(const char *a, const char *b, size_t n);

void *memset32(void *dst, uint32_t value, size_t count);
void *memcpy32(void *dst, const void *src, size_t count);
