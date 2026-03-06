#include "log.h"
#include "serial.h"

#include "stdlib/stdio.h"
#include "stdlib/stdarg.h"

#define LOG_BUFFER_SIZE 256
#define PANIC_BUFFER_SIZE 256

void log(const char *fmt, ...) 
{
    char buf[LOG_BUFFER_SIZE];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    serial_puts(buf);
    serial_puts("\n");
}

void panic(const char *fmt, ...) 
{
    char buf[PANIC_BUFFER_SIZE];
    va_list ap;

    serial_puts("PANIC: ");

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    serial_puts(buf);

    asm volatile ("cli");
    for (;;)
        asm volatile ("hlt");
}
