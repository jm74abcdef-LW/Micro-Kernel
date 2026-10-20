#include "gdt.h"

#include <stdint.h>

// A flat memory model: every segment covers the whole 4 GiB address space,
// so segmentation is effectively disabled. The GDT is still required to run
// in protected mode, and we cannot rely on the one left by the bootloader.

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;  // high nibble: flags, low nibble: limit bits 16-19
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Access byte: present, privilege ring, code/data, executable, readable/writable
#define GDT_KERNEL_CODE_ACCESS 0x9A
#define GDT_KERNEL_DATA_ACCESS 0x92
#define GDT_USER_CODE_ACCESS   0xFA
#define GDT_USER_DATA_ACCESS   0xF2

// Flags: 4 KiB granularity, 32-bit protected mode segment
#define GDT_FLAGS_32BIT_4K 0xC0

static struct gdt_entry gdt[5];
static struct gdt_ptr gdt_pointer;

extern void gdt_flush(const struct gdt_ptr *ptr);

static void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;
    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].granularity = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    gdt[index].access = access;
}

void gdt_init(void) {
    gdt_set_entry(0, 0, 0, 0, 0);  // the null descriptor is mandatory
    gdt_set_entry(1, 0, 0xFFFFF, GDT_KERNEL_CODE_ACCESS, GDT_FLAGS_32BIT_4K);  // 0x08
    gdt_set_entry(2, 0, 0xFFFFF, GDT_KERNEL_DATA_ACCESS, GDT_FLAGS_32BIT_4K);  // 0x10
    gdt_set_entry(3, 0, 0xFFFFF, GDT_USER_CODE_ACCESS, GDT_FLAGS_32BIT_4K);    // 0x18
    gdt_set_entry(4, 0, 0xFFFFF, GDT_USER_DATA_ACCESS, GDT_FLAGS_32BIT_4K);    // 0x20

    gdt_pointer.limit = sizeof(gdt) - 1;
    gdt_pointer.base = (uint32_t) &gdt;
    gdt_flush(&gdt_pointer);
}
