#pragma once

#include "stddef.h"
#include "stdarg.h"

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int snprintf(char *str, size_t size, const char *fmt, ...);
