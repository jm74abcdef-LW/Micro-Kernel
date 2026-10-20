#include <stdint.h>

#include "cpu.h"
#include "gdt.h"
#include "idt.h"
#include "io.h"
#include "kernel.h"
#include "keyboard.h"
#include "kprintf.h"
#include "multiboot.h"
#include "panic.h"
#include "pic.h"
#include "serial.h"
#include "shell.h"
#include "timer.h"
#include "vga.h"

static void print_banner(void) {
    static const char *const logo[] = {
        " _                 ___  ____  ",
        "| |    ___  ___   / _ \\/ ___| ",
        "| |   / _ \\/ _ \\ | | | \\___ \\ ",
        "| |__|  __/ (_) || |_| |___) |",
        "|_____\\___|\\___/  \\___/|____/ ",
    };

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    for (unsigned i = 0; i < sizeof(logo) / sizeof(logo[0]); i++) {
        kprintf("  %s\n", logo[i]);
    }
    vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    kprintf("  %s v%s - a hobby x86 kernel\n\n", KERNEL_NAME, KERNEL_VERSION);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

// Prints a boot step as "[ OK ] message"
static void status(const char *message) {
    uint8_t saved = vga_get_attr();
    kprint("[ ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    kprint("OK");
    vga_set_attr(saved);
    kprintf(" ] %s\n", message);
}

static void info(const char *label, const char *value) {
    uint8_t saved = vga_get_attr();
    kprintf("[INFO] %s: ", label);
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprint(value);
    vga_set_attr(saved);
    kputchar('\n');
}

void kernel_main(uint32_t magic, const struct multiboot_info *mbi) {
    vga_init();
    bool has_serial = serial_init();
    print_banner();

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("Not loaded by a Multiboot bootloader (magic 0x%08x)", magic);
    }
    status("Multiboot header loaded");
    status("VGA text mode ready");
    if (has_serial) status("Serial console on COM1");

    gdt_init();
    status("GDT loaded (flat 4 GiB segments)");
    idt_init();
    status("IDT loaded, CPU exceptions handled");
    pic_remap(IRQ_BASE, IRQ_BASE + 8);
    status("PIC remapped to vectors 32-47");
    timer_init(TIMER_FREQUENCY);
    status("PIT timer running at 100 Hz");
    keyboard_init();
    status("PS/2 keyboard ready");
    interrupts_enable();
    status("Interrupts enabled");

    struct cpu_info cpu;
    cpu_identify(&cpu);
    info("CPU vendor detected", cpu.vendor);
    if (cpu.brand[0]) info("CPU model", cpu.brand);
    if (mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
        info("Bootloader", (const char *) mbi->boot_loader_name);
    }
    if (mbi->flags & MULTIBOOT_INFO_MEMORY) {
        kprintf("[INFO] Memory: %u MiB\n", (mbi->mem_upper + 1024) / 1024);
    }

    kprint("\nSystem ready. Type 'help' to list the available commands.\n\n");
    shell_run(mbi);
}
