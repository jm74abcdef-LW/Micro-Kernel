#include "kstring.h"

#include <stdint.h>

void *memset(void *dest, int value, size_t count) {
    uint8_t *d = dest;
    while (count--) *d++ = (uint8_t) value;
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (count--) *d++ = *s++;
    return dest;
}

void *memmove(void *dest, const void *src, size_t count) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    if (d < s) {
        while (count--) *d++ = *s++;
    } else {
        // Copy backwards so overlapping regions are not clobbered
        while (count--) d[count] = s[count];
    }
    return dest;
}

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char) *a - (unsigned char) *b;
}

int strncmp(const char *a, const char *b, size_t count) {
    for (; count; count--, a++, b++) {
        if (*a != *b || !*a) return (unsigned char) *a - (unsigned char) *b;
    }
    return 0;
}

char *strchr(const char *str, int c) {
    for (; *str; str++) {
        if (*str == (char) c) return (char *) str;
    }
    return c ? NULL : (char *) str;
}
