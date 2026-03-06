#include "string.h"

void *memset(void *dst, int value, size_t size)
{
    unsigned char *p = (unsigned char *)dst;
    unsigned char byte = (unsigned char)value;

    for (size_t i = 0; i < size; i++)
        p[i] = byte;

    return dst;
}

void *memcpy(void *dst, const void *src, size_t size)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    for (size_t i = 0; i < size; i++)
        d[i] = s[i];

    return dst;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }

    return 0;
}

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

char *strncpy(char *dst, const char *src, size_t n)
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
    a_len = strnlen(a, cap);
    b_len = strnlen(b, cap);

    if (a_len == cap || b_len == cap)
        return false;
    if (a_len != b_len)
        return false;

    return strncmp(a, b, a_len) == 0;
}
