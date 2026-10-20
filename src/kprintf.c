#include "kprintf.h"

#include <stdbool.h>
#include <stdint.h>

#include "kstring.h"
#include "serial.h"
#include "vga.h"

void kputchar(char c) {
    vga_putchar(c);

    // Translate to what a serial terminal expects
    if (c == '\n') {
        serial_putchar('\r');
    } else if (c == '\b') {
        serial_putchar('\b');
        serial_putchar(' ');
    }
    serial_putchar((unsigned char) c < 0x80 ? c : '?');
}

void kprint(const char *str) {
    while (*str) kputchar(*str++);
}

void kclear(void) {
    vga_clear();
    // ANSI sequence: clear the screen and move the cursor home
    const char *ansi = "\033[2J\033[H";
    while (*ansi) serial_putchar(*ansi++);
}

// Writes `len` characters of `str`, padded to `width`
static void emit(const char *str, int len, int width, bool left, char pad) {
    int fill = width > len ? width - len : 0;
    if (!left) {
        for (int i = 0; i < fill; i++) kputchar(pad);
    }
    for (int i = 0; i < len; i++) kputchar(str[i]);
    if (left) {
        for (int i = 0; i < fill; i++) kputchar(' ');
    }
}

// Converts `value` to text in `out` (not NUL-terminated) and returns its length
static int utoa(uint32_t value, unsigned base, bool upper, char *out) {
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[32];
    int len = 0;
    do {
        tmp[len++] = digits[value % base];
        value /= base;
    } while (value);
    for (int i = 0; i < len; i++) out[i] = tmp[len - 1 - i];
    return len;
}

void vkprintf(const char *fmt, va_list args) {
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            kputchar(*fmt);
            continue;
        }
        fmt++;

        bool left = false;
        char pad = ' ';
        int width = 0;
        for (;; fmt++) {
            if (*fmt == '-') left = true;
            else if (*fmt == '0') pad = '0';
            else break;
        }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        while (*fmt == 'l') fmt++;  // long and int are both 32-bit here

        char buf[34];
        int len;
        switch (*fmt) {
        case 'c':
            buf[0] = (char) va_arg(args, int);
            emit(buf, 1, width, left, ' ');
            break;
        case 's': {
            const char *str = va_arg(args, const char *);
            if (!str) str = "(null)";
            emit(str, strlen(str), width, left, ' ');
            break;
        }
        case 'd':
        case 'i': {
            int32_t value = va_arg(args, int32_t);
            uint32_t magnitude = value < 0 ? -(uint32_t) value : (uint32_t) value;
            len = utoa(magnitude, 10, false, buf);
            if (value < 0) {
                // The sign goes before zero padding, but after space padding
                if (pad == '0') {
                    kputchar('-');
                    width--;
                } else {
                    memmove(buf + 1, buf, len++);
                    buf[0] = '-';
                }
            }
            emit(buf, len, width, left, pad);
            break;
        }
        case 'u':
            len = utoa(va_arg(args, uint32_t), 10, false, buf);
            emit(buf, len, width, left, pad);
            break;
        case 'x':
        case 'X':
            len = utoa(va_arg(args, uint32_t), 16, *fmt == 'X', buf);
            emit(buf, len, width, left, pad);
            break;
        case 'p':
            buf[0] = '0';
            buf[1] = 'x';
            len = 2 + utoa((uintptr_t) va_arg(args, void *), 16, false, buf + 2);
            emit(buf, len, width, left, ' ');
            break;
        case '%':
            kputchar('%');
            break;
        case '\0':
            return;
        default:
            kputchar('%');
            kputchar(*fmt);
        }
    }
}

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vkprintf(fmt, args);
    va_end(args);
}
