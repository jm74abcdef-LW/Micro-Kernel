#include "pic.h"

#include "io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define PIC_EOI       0x20
#define PIC_READ_ISR  0x0B
#define ICW1_INIT     0x11  // start initialization, ICW4 will follow
#define ICW4_8086     0x01

// By default the master PIC delivers IRQs 0-7 on vectors 8-15, which collide
// with CPU exceptions. Remapping moves them out of the way.
void pic_remap(uint8_t master_offset, uint8_t slave_offset) {
    outb(PIC1_COMMAND, ICW1_INIT);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT);
    io_wait();
    outb(PIC1_DATA, master_offset);  // ICW2: vector offsets
    io_wait();
    outb(PIC2_DATA, slave_offset);
    io_wait();
    outb(PIC1_DATA, 1 << 2);         // ICW3: slave is wired to the master's IRQ 2
    io_wait();
    outb(PIC2_DATA, 2);              // ICW3: slave cascade identity
    io_wait();
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Mask every line: drivers unmask the IRQs they handle
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_unmask(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_DATA, inb(PIC2_DATA) & ~(1 << (irq - 8)));
        irq = 2;  // the cascade line must be open too
    }
    outb(PIC1_DATA, inb(PIC1_DATA) & ~(1 << irq));
}

bool pic_handle_spurious(uint8_t irq) {
    if (irq != 7 && irq != 15) return false;

    // A real IRQ shows up in the In-Service Register, a spurious one does not
    uint16_t port = irq == 7 ? PIC1_COMMAND : PIC2_COMMAND;
    outb(port, PIC_READ_ISR);
    if (inb(port) & (1 << (irq & 7))) return false;

    // The master does not know the slave's IRQ was spurious
    if (irq == 15) outb(PIC1_COMMAND, PIC_EOI);
    return true;
}
