#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Hardware IRQs 0-15 are remapped to vectors 32-47, right after the CPU exceptions.
#define IRQ_BASE      32
#define IRQ_TIMER     (IRQ_BASE + 0)
#define IRQ_KEYBOARD  (IRQ_BASE + 1)

// CPU state saved by the assembly stubs in interrupts.s, in stack order.
struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pushed by pusha
    uint32_t int_no, err_code;                       // pushed by the stub
    uint32_t eip, cs, eflags;                        // pushed by the CPU
    uint32_t useresp, ss;                            // only on a privilege change
};

typedef void (*interrupt_handler_t)(struct registers *regs);

void idt_init(void);
void register_interrupt_handler(uint8_t vector, interrupt_handler_t handler);

#endif
