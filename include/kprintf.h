#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdarg.h>

// Console output, sent to both the VGA screen and the serial port.

void kputchar(char c);
void kprint(const char *str);
void kclear(void);

// Supports %c %s %d %i %u %x %X %p %%, the '-' and '0' flags and a field width.
void kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void vkprintf(const char *fmt, va_list args);

#endif
