#include <stdint.h>
#include <stddef.h>

// Standard physical address for the VGA text mode buffer
#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_WHITE = 15,
};

size_t terminal_row = 0;
size_t terminal_column = 0;
uint16_t* terminal_buffer = (uint16_t*) VGA_ADDRESS;

// Constructs a VGA entry (16 bits): 
// 8 high bits for the color, 8 low bits for the ASCII character
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

void terminal_putchar(char c, uint8_t color) {
    // Basic carriage return handling to prevent video memory overflow
    if (c == '\n') {
        terminal_row++;
        terminal_column = 0;
        return;
    }
    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    terminal_buffer[index] = vga_entry(c, color);
    
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) terminal_row = 0;
    }
}

void terminal_writestring(const char* data, uint8_t color) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        terminal_putchar(data[i], color);
    }
}

// Queries the processor via the CPUID instruction (EAX=0) to retrieve the Vendor ID.
// The 12-character string is returned spread across the EBX, EDX, and ECX hardware registers.
void get_cpu_vendor(char *vendor) {
    uint32_t eax = 0, ebx, ecx, edx;
    
    // Inline assembly: mapping C variables directly to CPU registers
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));
    
    ((uint32_t*)vendor)[0] = ebx;
    ((uint32_t*)vendor)[1] = edx;
    ((uint32_t*)vendor)[2] = ecx;
    vendor[12] = '\0';
}

void kernel_main(void) {
    // Initialization: Clearing the VGA frame buffer to remove BIOS/Bootloader artifacts
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', VGA_COLOR_BLACK);
        }
    }

    terminal_writestring("LeoOS Kernel Initialized...\n", VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("[OK] Multiboot Header loaded\n", VGA_COLOR_GREEN);
    terminal_writestring("[OK] VGA Text Mode ready\n", VGA_COLOR_GREEN);

    char vendor[13];
    get_cpu_vendor(vendor);
    terminal_writestring("[INFO] CPU Vendor detected : ", VGA_COLOR_WHITE);
    terminal_writestring(vendor, VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("\n", VGA_COLOR_WHITE);
    
    terminal_writestring("\nSystem Ready. Awaiting commands...", VGA_COLOR_WHITE);
}
