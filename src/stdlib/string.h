#pragma once

#include "stddef.h"
#include "stdbool.h"

void *memset(void *dst, int value, size_t size);
void *memcpy(void *dst, const void *src, size_t size);
int memcmp(const void *s1, const void *s2, size_t n);
int strncmp(const char *a, const char *b, size_t n);
size_t strnlen(const char *str, size_t n);
char *strncpy(char *dst, const char *src, size_t n);
bool strneq(const char *a, const char *b, size_t n);
