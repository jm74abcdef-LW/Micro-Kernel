#ifndef SHELL_H
#define SHELL_H

#include "multiboot.h"

void shell_run(const struct multiboot_info *mbi) __attribute__((noreturn));

#endif
