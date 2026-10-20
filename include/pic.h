#ifndef PIC_H
#define PIC_H

#include <stdbool.h>
#include <stdint.h>

// Driver for the two cascaded 8259 Programmable Interrupt Controllers.

void pic_remap(uint8_t master_offset, uint8_t slave_offset);
void pic_send_eoi(uint8_t irq);
void pic_unmask(uint8_t irq);

// Returns true if IRQ 7/15 was spurious (and acknowledges the master if needed).
bool pic_handle_spurious(uint8_t irq);

#endif
