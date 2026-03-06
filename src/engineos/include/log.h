#pragma once

void log(const char *fmt, ...);
void panic(const char *fmt, ...) __attribute__((noreturn));

#define PANIC(fmt, ...) \
    panic("in '%s' [%s:%d]: " fmt, __func__, __FILE__, __LINE__, ##__VA_ARGS__)
