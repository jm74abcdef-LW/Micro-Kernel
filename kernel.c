/* kernel.c */
#include <stdint.h>

#define VGA_ADDRESS 0xB8000

#define VGA_WIDTH 80
#define VGA_HEIGHT 25


enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_WHITE = 15,
    VGA_COLOR_LIGHT_CYAN = 11,
};


static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
{
    return (uint16_t) uc | (uint16_t) color << 8;
}


void terminal_writestring(const char* data)
{

    volatile uint16_t* vga_buffer = (uint16_t*) VGA_ADDRESS;


    for (int i = 0; data[i] != '\0'; i++) {

        vga_buffer[i] = vga_entry(data[i], VGA_COLOR_LIGHT_CYAN);
    }
}


void kernel_main(void)
{
    terminal_writestring("Ez");
}
