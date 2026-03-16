#include "stddef.h"

int strncmp(const char *a, const char *b, size_t n)
{
    size_t i = 0;

    while (i < n && a[i] && a[i] == b[i])
        i++;

    if (i == n)
        return 0;

    return (unsigned char)a[i] - (unsigned char)b[i];
}

size_t strnlen(const char *str, size_t n)
{
    size_t i = 0;

    if (!str)
        return 0;

    while (i < n && str[i] != '\0')
        i++;

    return i;
}

char *strncpy(char *restrict dst, const char *restrict src, size_t n)
{
    size_t i = 0;

    while (i < n && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    while (i < n)
        dst[i++] = '\0';

    return dst;
}

bool strneq(const char *a, const char *b, size_t n)
{
    size_t cap;
    size_t a_len;
    size_t b_len;

    if (!a || !b)
        return false;

    cap = (n == (size_t)-1) ? n : n + 1u;
    a_len = __builtin_strnlen(a, cap);
    b_len = __builtin_strnlen(b, cap);

    if (a_len == cap || b_len == cap)
        return false;
    if (a_len != b_len)
        return false;

    return __builtin_strncmp(a, b, a_len) == 0;
}
