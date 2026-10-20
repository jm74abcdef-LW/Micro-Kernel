#ifndef VGA_H
#define VGA_H

#include <stdint.h>

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
};

void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_set_color(enum vga_color fg, enum vga_color bg);

// Raw attribute byte (background in the high nibble, foreground in the low one),
// handy to save and restore the current color.
uint8_t vga_get_attr(void);
void vga_set_attr(uint8_t attr);

#endif
