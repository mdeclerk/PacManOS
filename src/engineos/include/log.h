#pragma once

void log(const char *fmt, ...);
[[noreturn]] void panic(const char *fmt, ...);

#define PANIC(fmt, ...) \
    panic("in '%s' [%s:%d]: " fmt, __func__, __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
