#include "idt.h"

#include "gdt.h"
#include "panic.h"
#include "pic.h"

#define IDT_ENTRIES 256
#define ISR_STUBS   48

// Present, ring 0, 32-bit interrupt gate (interrupts disabled on entry)
#define IDT_INTERRUPT_GATE 0x8E

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idt_pointer;
static interrupt_handler_t handlers[IDT_ENTRIES];

extern void *isr_stub_table[ISR_STUBS];
extern void idt_load(const struct idt_ptr *ptr);

static void idt_set_gate(uint8_t vector, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[vector].base_low = base & 0xFFFF;
    idt[vector].base_high = (base >> 16) & 0xFFFF;
    idt[vector].selector = selector;
    idt[vector].zero = 0;
    idt[vector].flags = flags;
}

void idt_init(void) {
    for (int i = 0; i < ISR_STUBS; i++) {
        idt_set_gate(i, (uint32_t) isr_stub_table[i], GDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    }
    idt_pointer.limit = sizeof(idt) - 1;
    idt_pointer.base = (uint32_t) &idt;
    idt_load(&idt_pointer);
}

void register_interrupt_handler(uint8_t vector, interrupt_handler_t handler) {
    handlers[vector] = handler;
}

// Called by isr_common (interrupts.s) for every interrupt
void isr_handler(struct registers *regs) {
    uint32_t vector = regs->int_no;

    if (vector < IRQ_BASE) {
        // CPU exception: nothing can recover from it yet unless a handler says so
        if (handlers[vector]) handlers[vector](regs);
        else panic_exception(regs);
        return;
    }

    uint8_t irq = vector - IRQ_BASE;
    if (pic_handle_spurious(irq)) return;
    if (handlers[vector]) handlers[vector](regs);
    pic_send_eoi(irq);
}
