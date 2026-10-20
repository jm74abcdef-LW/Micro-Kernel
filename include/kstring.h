#ifndef KSTRING_H
#define KSTRING_H

#include <stddef.h>

// Minimal freestanding replacements for <string.h>.
// GCC may also emit calls to memset/memcpy/memmove on its own, so they must exist.

void *memset(void *dest, int value, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
void *memmove(void *dest, const void *src, size_t count);
size_t strlen(const char *str);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t count);
char *strchr(const char *str, int c);

#endif
