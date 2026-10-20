#include "shell.h"

#include <stdbool.h>
#include <stdint.h>

#include "cpu.h"
#include "io.h"
#include "kernel.h"
#include "keyboard.h"
#include "kprintf.h"
#include "kstring.h"
#include "power.h"
#include "serial.h"
#include "timer.h"
#include "vga.h"

#define LINE_MAX     256
#define MAX_ARGS     16
#define HISTORY_SIZE 16

#define CTRL_C 3
#define CTRL_L 12

struct command {
    const char *name;
    const char *usage;
    const char *help;
    void (*run)(int argc, char **argv);
};

static void cmd_help(int argc, char **argv);
static void cmd_clear(int argc, char **argv);
static void cmd_echo(int argc, char **argv);
static void cmd_about(int argc, char **argv);
static void cmd_cpu(int argc, char **argv);
static void cmd_mem(int argc, char **argv);
static void cmd_uptime(int argc, char **argv);
static void cmd_sleep(int argc, char **argv);
static void cmd_color(int argc, char **argv);
static void cmd_layout(int argc, char **argv);
static void cmd_history(int argc, char **argv);
static void cmd_crash(int argc, char **argv);
static void cmd_reboot(int argc, char **argv);
static void cmd_shutdown(int argc, char **argv);

static const struct command commands[] = {
    { "help", "", "Show this list", cmd_help },
    { "clear", "", "Clear the screen (also Ctrl+L)", cmd_clear },
    { "echo", "<text>", "Print a line of text", cmd_echo },
    { "about", "", "Show kernel version and build information", cmd_about },
    { "cpu", "", "Show processor information (CPUID)", cmd_cpu },
    { "mem", "", "Show the memory map given by the bootloader", cmd_mem },
    { "uptime", "", "Show the time elapsed since boot", cmd_uptime },
    { "sleep", "<ms>", "Wait for the given number of milliseconds", cmd_sleep },
    { "color", "<fg> [bg]", "Change the text colors", cmd_color },
    { "layout", "[fr|us]", "Show or change the keyboard layout", cmd_layout },
    { "history", "", "List previous commands", cmd_history },
    { "crash", "", "Trigger a division by zero to test the panic screen", cmd_crash },
    { "reboot", "", "Restart the machine", cmd_reboot },
    { "shutdown", "", "Power off the machine", cmd_shutdown },
};

#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))

static const char *const color_names[16] = {
    "black", "blue", "green", "cyan", "red", "magenta", "brown", "lightgrey",
    "darkgrey", "lightblue", "lightgreen", "lightcyan", "lightred", "lightmagenta", "yellow", "white",
};

static const struct multiboot_info *boot_info;
static char history[HISTORY_SIZE][LINE_MAX];
static int history_count;

// ---------------------------------------------------------------------------
// Line editing
// ---------------------------------------------------------------------------

static void print_prompt(void) {
    uint8_t saved = vga_get_attr();
    enum vga_color bg = saved >> 4;  // keep the background chosen with `color`
    vga_set_color(VGA_COLOR_LIGHT_GREEN, bg);
    kprint("leoos");
    vga_set_color(VGA_COLOR_WHITE, bg);
    kprint("> ");
    vga_set_attr(saved);
}

// Blocks until a key arrives from the PS/2 keyboard or the serial port
static int read_key(void) {
    for (;;) {
        int key = keyboard_poll();
        if (key < 0) key = serial_poll();
        if (key >= 0) return key;
        // Sleep until the next interrupt (at worst the next timer tick)
        cpu_halt();
    }
}

// Erases the current input and replaces it with `text`
static void replace_line(char *buf, size_t *len, const char *text) {
    while (*len) {
        kputchar('\b');
        (*len)--;
    }
    for (; *text && *len < LINE_MAX - 1; text++) {
        buf[(*len)++] = *text;
        kputchar(*text);
    }
}

// Completes the command name being typed, or lists the candidates
static void complete(char *buf, size_t *len) {
    buf[*len] = '\0';
    if (strchr(buf, ' ')) return;

    const struct command *match = NULL;
    int matches = 0;
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if (strncmp(commands[i].name, buf, *len) == 0) {
            match = &commands[i];
            matches++;
        }
    }

    if (matches == 1) {
        for (const char *c = match->name + *len; *c; c++) {
            buf[(*len)++] = *c;
            kputchar(*c);
        }
        buf[(*len)++] = ' ';
        kputchar(' ');
    } else if (matches > 1) {
        kputchar('\n');
        for (size_t i = 0; i < COMMAND_COUNT; i++) {
            if (strncmp(commands[i].name, buf, *len) == 0) kprintf("%s  ", commands[i].name);
        }
        kputchar('\n');
        print_prompt();
        kprint(buf);
    }
}

static void read_line(char *buf) {
    size_t len = 0;
    int history_pos = history_count;  // one past the most recent entry

    for (;;) {
        int key = read_key();
        switch (key) {
        case '\n':
            kputchar('\n');
            buf[len] = '\0';
            return;
        case '\b':
            if (len > 0) {
                len--;
                kputchar('\b');
            }
            break;
        case '\t':
            complete(buf, &len);
            break;
        case CTRL_C:
            kprint("^C\n");
            buf[0] = '\0';
            return;
        case CTRL_L:
            buf[len] = '\0';
            kclear();
            print_prompt();
            kprint(buf);
            break;
        case KEY_UP:
            if (history_pos > 0) replace_line(buf, &len, history[--history_pos]);
            break;
        case KEY_DOWN:
            if (history_pos < history_count) {
                history_pos++;
                replace_line(buf, &len, history_pos == history_count ? "" : history[history_pos]);
            }
            break;
        default:
            // Printable ASCII and CP437 characters only
            if (key >= 0x20 && key <= 0xFF && key != 0x7F && len < LINE_MAX - 1) {
                buf[len++] = (char) key;
                kputchar((char) key);
            }
        }
    }
}

static void history_add(const char *line) {
    if (!*line) return;
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) return;

    if (history_count == HISTORY_SIZE) {
        memmove(history[0], history[1], (HISTORY_SIZE - 1) * LINE_MAX);
        history_count--;
    }
    memcpy(history[history_count], line, strlen(line) + 1);
    history_count++;
}

// Splits `line` in place on spaces and returns the number of arguments
static int tokenize(char *line, char **argv) {
    int argc = 0;
    while (*line && argc < MAX_ARGS) {
        while (*line == ' ') *line++ = '\0';
        if (!*line) break;
        argv[argc++] = line;
        while (*line && *line != ' ') line++;
    }
    return argc;
}

static void execute(char *line) {
    char *argv[MAX_ARGS];
    int argc = tokenize(line, argv);
    if (argc == 0) return;

    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].run(argc, argv);
            return;
        }
    }
    kprintf("%s: command not found. Type 'help' for a list of commands.\n", argv[0]);
}

void shell_run(const struct multiboot_info *mbi) {
    char line[LINE_MAX];
    boot_info = mbi;

    for (;;) {
        print_prompt();
        read_line(line);
        history_add(line);
        execute(line);
    }
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

static bool parse_uint(const char *str, uint32_t *out) {
    uint32_t value = 0;
    if (!*str) return false;
    for (; *str; str++) {
        if (*str < '0' || *str > '9') return false;
        value = value * 10 + (*str - '0');
    }
    *out = value;
    return true;
}

static int parse_color(const char *name) {
    for (int i = 0; i < 16; i++) {
        if (strcmp(color_names[i], name) == 0) return i;
    }
    return -1;
}

static void cmd_help(int argc, char **argv) {
    (void) argc;
    (void) argv;
    kprint("Available commands:\n");
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        kprintf("  %-9s%-11s %s\n", commands[i].name, commands[i].usage, commands[i].help);
    }
    kprint("Keys: Up/Down for history, Tab to complete, Ctrl+C to cancel the line.\n");
}

static void cmd_clear(int argc, char **argv) {
    (void) argc;
    (void) argv;
    kclear();
}

static void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        kprintf(i > 1 ? " %s" : "%s", argv[i]);
    }
    kputchar('\n');
}

static void cmd_about(int argc, char **argv) {
    (void) argc;
    (void) argv;
    kprintf("%s v%s - 32-bit x86 Multiboot kernel\n", KERNEL_NAME, KERNEL_VERSION);
    kprintf("Built on %s at %s with GCC %s\n", __DATE__, __TIME__, __VERSION__);
}

static void cmd_cpu(int argc, char **argv) {
    (void) argc;
    (void) argv;
    struct cpu_info cpu;
    cpu_identify(&cpu);
    kprintf("Vendor   : %s\n", cpu.vendor);
    kprintf("Model    : %s\n", cpu.brand[0] ? cpu.brand : "(no brand string)");
    kprintf("Family %u, model %u, stepping %u\n", cpu.family, cpu.model, cpu.stepping);
    kprint("Features : ");
    cpu_print_features(&cpu);
    kputchar('\n');
}

static const char *memory_type(uint32_t type) {
    switch (type) {
    case MULTIBOOT_MEMORY_AVAILABLE: return "available";
    case MULTIBOOT_MEMORY_RESERVED: return "reserved";
    case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE: return "ACPI reclaimable";
    case MULTIBOOT_MEMORY_NVS: return "ACPI NVS";
    case MULTIBOOT_MEMORY_BADRAM: return "bad RAM";
    default: return "unknown";
    }
}

static void cmd_mem(int argc, char **argv) {
    (void) argc;
    (void) argv;
    extern char kernel_start[], kernel_end[];

    uint32_t kernel_size = (uint32_t) (kernel_end - kernel_start);
    kprintf("Kernel image : 0x%08x - 0x%08x (%u KiB)\n",
            (uint32_t) kernel_start, (uint32_t) kernel_end, (kernel_size + 1023) / 1024);

    if (boot_info->flags & MULTIBOOT_INFO_MEMORY) {
        kprintf("Lower memory : %u KiB\n", boot_info->mem_lower);
        kprintf("Upper memory : %u KiB (%u MiB)\n", boot_info->mem_upper, boot_info->mem_upper / 1024);
    }

    if (!(boot_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        kprint("No memory map provided by the bootloader.\n");
        return;
    }

    kprint("Memory map:\n");
    uint32_t available_kib = 0;
    uintptr_t addr = boot_info->mmap_addr;
    uintptr_t end = addr + boot_info->mmap_length;
    while (addr < end) {
        const struct multiboot_mmap_entry *entry = (const struct multiboot_mmap_entry *) addr;
        uint64_t last = entry->addr + entry->len - 1;
        uint32_t size_kib = (uint32_t) (entry->len >> 10);

        kprintf("  0x%08x%08x - 0x%08x%08x  %-16s %u KiB\n",
                (uint32_t) (entry->addr >> 32), (uint32_t) entry->addr,
                (uint32_t) (last >> 32), (uint32_t) last,
                memory_type(entry->type), size_kib);
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) available_kib += size_kib;

        addr += entry->size + sizeof(entry->size);
    }
    kprintf("Total available: %u MiB\n", available_kib / 1024);
}

static void cmd_uptime(int argc, char **argv) {
    (void) argc;
    (void) argv;
    uint32_t ticks = timer_ticks();
    uint32_t seconds = ticks / timer_frequency();
    kprintf("Up %u:%02u:%02u (%u ticks at %u Hz)\n",
            seconds / 3600, (seconds / 60) % 60, seconds % 60, ticks, timer_frequency());
}

static void cmd_sleep(int argc, char **argv) {
    uint32_t ms;
    if (argc != 2 || !parse_uint(argv[1], &ms)) {
        kprint("usage: sleep <ms>\n");
        return;
    }
    timer_sleep(ms);
}

static void cmd_color(int argc, char **argv) {
    int fg = argc >= 2 ? parse_color(argv[1]) : -1;
    int bg = argc >= 3 ? parse_color(argv[2]) : VGA_COLOR_BLACK;

    if (argc < 2 || argc > 3 || fg < 0 || bg < 0) {
        kprint("usage: color <fg> [bg]\ncolors:");
        for (int i = 0; i < 16; i++) kprintf(" %s", color_names[i]);
        kputchar('\n');
        return;
    }
    vga_set_color(fg, bg);
}

static void cmd_layout(int argc, char **argv) {
    if (argc == 1) {
        kprintf("Keyboard layout: %s (available: %s)\n", keyboard_layout(), keyboard_layouts());
    } else if (argc != 2 || !keyboard_set_layout(argv[1])) {
        kprintf("usage: layout [%s]\n", keyboard_layouts());
    }
}

static void cmd_history(int argc, char **argv) {
    (void) argc;
    (void) argv;
    for (int i = 0; i < history_count; i++) {
        kprintf("%3d  %s\n", i + 1, history[i]);
    }
}

static void cmd_crash(int argc, char **argv) {
    (void) argc;
    (void) argv;
    // Written in assembly: a C division by zero is undefined behaviour that
    // the compiler is free to optimise away.
    __asm__ volatile("xor %%edx, %%edx\n\t"
                     "xor %%ecx, %%ecx\n\t"
                     "div %%ecx"
                     : : : "eax", "ecx", "edx");
}

static void cmd_reboot(int argc, char **argv) {
    (void) argc;
    (void) argv;
    kprint("Rebooting...\n");
    power_reboot();
}

static void cmd_shutdown(int argc, char **argv) {
    (void) argc;
    (void) argv;
    kprint("Shutting down...\n");
    power_shutdown();
}
