#include "panic.h"

#include <stdarg.h>
#include <stddef.h>

#include "io.h"
#include "kprintf.h"
#include "vga.h"

static const char *const exception_names[32] = {
    "Division by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 floating-point exception",
    "Alignment check",
    "Machine check",
    "SIMD floating-point exception",
    "Virtualization exception",
    "Control protection exception",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor injection exception",
    "VMM communication exception",
    "Security exception",
    "Reserved",
};

static void panic_screen(void) {
    interrupts_disable();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    kclear();
    kprint("\n  *** KERNEL PANIC ***\n\n");
}

static void __attribute__((noreturn)) halt_forever(void) {
    kprint("\n  The system has been halted.\n");
    for (;;) {
        interrupts_disable();
        cpu_halt();
    }
}

void panic(const char *fmt, ...) {
    panic_screen();
    kprint("  ");
    va_list args;
    va_start(args, fmt);
    vkprintf(fmt, args);
    va_end(args);
    kputchar('\n');
    halt_forever();
}

void panic_exception(const struct registers *regs) {
    panic_screen();

    kprintf("  Exception %u: %s\n", regs->int_no, exception_names[regs->int_no & 31]);
    kprintf("  Error code: 0x%08x\n\n", regs->err_code);

    // The CPU did not switch stacks (ring 0 -> ring 0), so the interrupted ESP
    // is right after the EIP/CS/EFLAGS frame it pushed.
    uint32_t esp = (uint32_t) regs + offsetof(struct registers, useresp);

    kprintf("  EAX=%08x  EBX=%08x  ECX=%08x  EDX=%08x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    kprintf("  ESI=%08x  EDI=%08x  EBP=%08x  ESP=%08x\n", regs->esi, regs->edi, regs->ebp, esp);
    kprintf("  EIP=%08x  CS=%04x  DS=%04x  EFLAGS=%08x\n", regs->eip, regs->cs, regs->ds, regs->eflags);

    if (regs->int_no == 14) {
        uint32_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        kprintf("  CR2=%08x (faulting address)\n", cr2);
    }
    halt_forever();
}
