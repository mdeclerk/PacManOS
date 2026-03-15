#pragma once

#include "stddef.h"
#include "stdarg.h"

int vsnprintf(char *restrict str, size_t size, const char *restrict fmt, va_list ap);
int snprintf(char *restrict str, size_t size, const char *restrict fmt, ...);
