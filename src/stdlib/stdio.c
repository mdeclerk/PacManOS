#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"

struct out_buf {
    char *dst;
    size_t size;
    size_t pos;
};

static void out_char(struct out_buf *out, char c) {
    if (out->size != 0 && out->pos + 1 < out->size) {
        out->dst[out->pos] = c;
    }
    out->pos++;
}

static size_t utoa_base(char *tmp, size_t tmp_size, uint32_t value, unsigned base, bool upper) {
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    size_t i = 0;

    if (tmp_size == 0) {
        return 0;
    }

    if (value == 0) {
        tmp[i++] = '0';
        return i;
    }

    while (value != 0 && i < tmp_size) {
        tmp[i++] = digits[value % base];
        value /= base;
    }

    return i;
}

static void out_str(struct out_buf *out, const char *str) {
    const char *s = str ? str : "(null)";
    while (*s) {
        out_char(out, *s++);
    }
}

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap) {
    struct out_buf out = { str, size, 0 };
    const char *p = fmt;

    if (!str && size != 0) {
        return 0;
    }

    while (*p) {
        if (*p != '%') {
            out_char(&out, *p++);
            continue;
        }
        p++;

        if (*p == '%') {
            out_char(&out, *p++);
            continue;
        }

        enum {
            LEN_DEFAULT,
            LEN_LONG,
            LEN_SIZE
        } length = LEN_DEFAULT;

        if (*p == 'z') {
            length = LEN_SIZE;
            p++;
        } else if (*p == 'l') {
            length = LEN_LONG;
            p++;
        }

        switch (*p) {
        case 'c':
            out_char(&out, (char)va_arg(ap, int));
            p++;
            break;
        case 's':
            out_str(&out, va_arg(ap, const char *));
            p++;
            break;
        case 'p': {
            uintptr_t value = (uintptr_t)va_arg(ap, void *);
            char tmp[32];
            size_t len = utoa_base(tmp, sizeof(tmp), value, 16, false);
            out_char(&out, '0');
            out_char(&out, 'x');
            while (len > 0) {
                out_char(&out, tmp[--len]);
            }
            p++;
            break;
        }
        case 'x': {
            uint32_t value;
            char tmp[32];
            size_t len;
            if (length == LEN_LONG) {
                value = va_arg(ap, unsigned long);
            } else if (length == LEN_SIZE) {
                value = va_arg(ap, size_t);
            } else {
                value = va_arg(ap, unsigned int);
            }
            len = utoa_base(tmp, sizeof(tmp), value, 16, false);
            out_char(&out, '0');
            out_char(&out, 'x');
            while (len > 0) {
                out_char(&out, tmp[--len]);
            }
            p++;
            break;
        }
        case 'u': {
            uint32_t value;
            char tmp[32];
            size_t len;
            if (length == LEN_LONG) {
                value = va_arg(ap, unsigned long);
            } else if (length == LEN_SIZE) {
                value = va_arg(ap, size_t);
            } else {
                value = va_arg(ap, unsigned int);
            }
            len = utoa_base(tmp, sizeof(tmp), value, 10, false);
            while (len > 0) {
                out_char(&out, tmp[--len]);
            }
            p++;
            break;
        }
        case 'd': {
            int32_t value;
            char tmp[32];
            size_t len;
            if (length == LEN_LONG) {
                value = va_arg(ap, long);
            } else if (length == LEN_SIZE) {
                value = (int32_t)va_arg(ap, size_t);
            } else {
                value = va_arg(ap, int);
            }
            if (value < 0) {
                out_char(&out, '-');
                value = -value;
            }
            len = utoa_base(tmp, sizeof(tmp), (uint32_t)value, 10, false);
            while (len > 0) {
                out_char(&out, tmp[--len]);
            }
            p++;
            break;
        }
        default:
            out_char(&out, '%');
            if (*p) {
                out_char(&out, *p++);
            }
            break;
        }
    }

    if (out.size != 0) {
        size_t end = (out.pos < out.size) ? out.pos : out.size - 1;
        out.dst[end] = '\0';
    }

    return (int)out.pos;
}

int snprintf(char *str, size_t size, const char *fmt, ...) {
    int written;
    va_list ap;

    va_start(ap, fmt);
    written = vsnprintf(str, size, fmt, ap);
    va_end(ap);

    return written;
}
