#include "power.h"

#include "io.h"
#include "kprintf.h"

void power_reboot(void) {
    interrupts_disable();
    // Pulse the CPU reset line through the 8042 keyboard controller
    while (inb(0x64) & 0x02) {}
    outb(0x64, 0xFE);
    for (;;) cpu_halt();
}

void power_shutdown(void) {
    // There is no ACPI driver yet: use the shortcuts offered by emulators
    outw(0x604, 0x2000);   // QEMU
    outw(0xB004, 0x2000);  // Bochs and older QEMU
    outw(0x4004, 0x3400);  // VirtualBox

    // Still running: real hardware, let the user switch it off
    kprint("It is now safe to turn off your computer.\n");
    for (;;) {
        interrupts_disable();
        cpu_halt();
    }
}
