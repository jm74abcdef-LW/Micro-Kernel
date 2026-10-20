#ifndef PANIC_H
#define PANIC_H

#include "idt.h"

void panic(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));
void panic_exception(const struct registers *regs) __attribute__((noreturn));

#endif
