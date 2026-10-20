#ifndef IO_H
#define IO_H

#include <stdint.h>

// Thin wrappers around x86 instructions that C cannot express directly.

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Port 0x80 is unused after POST: writing to it gives slow devices time to settle.
static inline void io_wait(void) {
    outb(0x80, 0);
}

static inline void interrupts_enable(void) {
    __asm__ volatile("sti");
}

static inline void interrupts_disable(void) {
    __asm__ volatile("cli");
}

static inline void cpu_halt(void) {
    __asm__ volatile("hlt");
}

#endif
