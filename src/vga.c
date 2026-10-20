#include "vga.h"

#include <stddef.h>

#include "io.h"
#include "kstring.h"

// Standard physical address for the VGA text mode buffer
#define VGA_ADDRESS 0xB8000

// CRT controller ports, used to move the blinking hardware cursor
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA  0x3D5

static uint16_t *const buffer = (uint16_t *) VGA_ADDRESS;
static size_t row;
static size_t column;
static uint8_t attr;

// Constructs a VGA entry (16 bits):
// 8 high bits for the color, 8 low bits for the CP437 character
static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

static void update_cursor(void) {
    uint16_t pos = row * VGA_WIDTH + column;
    outb(VGA_CRTC_INDEX, 0x0F);
    outb(VGA_CRTC_DATA, pos & 0xFF);
    outb(VGA_CRTC_INDEX, 0x0E);
    outb(VGA_CRTC_DATA, pos >> 8);
}

// Shows the cursor as an underline (scanlines 14 to 15 of the character cell)
static void enable_cursor(void) {
    outb(VGA_CRTC_INDEX, 0x0A);
    outb(VGA_CRTC_DATA, (inb(VGA_CRTC_DATA) & 0xC0) | 14);
    outb(VGA_CRTC_INDEX, 0x0B);
    outb(VGA_CRTC_DATA, (inb(VGA_CRTC_DATA) & 0xE0) | 15);
}

// Moves every line up by one and blanks the last one
static void scroll(void) {
    memmove(buffer, buffer + VGA_WIDTH, (VGA_HEIGHT - 1) * VGA_WIDTH * sizeof(uint16_t));
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', attr);
    }
    row = VGA_HEIGHT - 1;
}

void vga_init(void) {
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    enable_cursor();
    // Clear the frame buffer to remove BIOS/bootloader artifacts
    vga_clear();
}

void vga_clear(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        buffer[i] = vga_entry(' ', attr);
    }
    row = 0;
    column = 0;
    update_cursor();
}

void vga_putchar(char c) {
    switch (c) {
    case '\n':
        column = 0;
        row++;
        break;
    case '\r':
        column = 0;
        break;
    case '\t':
        column = (column + 4) & ~(size_t) 3;
        break;
    case '\b':
        // Erase the previous cell, going back to the previous line if needed
        if (column > 0) {
            column--;
        } else if (row > 0) {
            row--;
            column = VGA_WIDTH - 1;
        }
        buffer[row * VGA_WIDTH + column] = vga_entry(' ', attr);
        break;
    default:
        buffer[row * VGA_WIDTH + column] = vga_entry(c, attr);
        column++;
    }

    if (column >= VGA_WIDTH) {
        column = 0;
        row++;
    }
    if (row >= VGA_HEIGHT) scroll();
    update_cursor();
}

void vga_set_color(enum vga_color fg, enum vga_color bg) {
    attr = fg | bg << 4;
}

uint8_t vga_get_attr(void) {
    return attr;
}

void vga_set_attr(uint8_t new_attr) {
    attr = new_attr;
}
