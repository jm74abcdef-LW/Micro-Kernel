#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>

// COM1 driver: mirrors the console so the kernel can be used with `qemu -nographic`.

bool serial_init(void);
bool serial_present(void);
void serial_putchar(char c);

// Returns the next character received (arrow keys are translated to KEY_* codes),
// or -1 if nothing is pending.
int serial_poll(void);

#endif
