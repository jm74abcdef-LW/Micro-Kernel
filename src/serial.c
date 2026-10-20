#include "serial.h"

#include <stdint.h>

#include "io.h"
#include "keyboard.h"

#define COM1 0x3F8

// UART register offsets
#define UART_DATA        0
#define UART_INT_ENABLE  1
#define UART_FIFO_CTRL   2
#define UART_LINE_CTRL   3
#define UART_MODEM_CTRL  4
#define UART_LINE_STATUS 5

#define LSR_DATA_READY 0x01
#define LSR_TX_EMPTY   0x20

static bool present;

bool serial_init(void) {
    outb(COM1 + UART_INT_ENABLE, 0x00);  // we poll, no interrupts
    outb(COM1 + UART_LINE_CTRL, 0x80);   // enable the divisor latch
    outb(COM1 + UART_DATA, 0x03);        // divisor 3 -> 38400 baud
    outb(COM1 + UART_INT_ENABLE, 0x00);
    outb(COM1 + UART_LINE_CTRL, 0x03);   // 8 bits, no parity, 1 stop bit
    outb(COM1 + UART_FIFO_CTRL, 0xC7);   // enable and clear the FIFOs

    // Loopback test: a missing UART will not echo the byte back
    outb(COM1 + UART_MODEM_CTRL, 0x1E);
    outb(COM1 + UART_DATA, 0xAE);
    if (inb(COM1 + UART_DATA) != 0xAE) {
        present = false;
        return false;
    }

    outb(COM1 + UART_MODEM_CTRL, 0x0F);  // back to normal operation
    present = true;
    return true;
}

bool serial_present(void) {
    return present;
}

void serial_putchar(char c) {
    if (!present) return;
    while (!(inb(COM1 + UART_LINE_STATUS) & LSR_TX_EMPTY)) {}
    outb(COM1 + UART_DATA, c);
}

int serial_poll(void) {
    // Small state machine decoding the ANSI arrow keys sent by terminals (ESC [ A)
    static int escape_state;

    if (!present || !(inb(COM1 + UART_LINE_STATUS) & LSR_DATA_READY)) return -1;
    uint8_t c = inb(COM1 + UART_DATA);

    if (escape_state == 1) {
        escape_state = c == '[' ? 2 : 0;
        return -1;
    }
    if (escape_state == 2) {
        escape_state = 0;
        switch (c) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        default: return -1;
        }
    }

    switch (c) {
    case 0x1B: escape_state = 1; return -1;
    case '\r': return '\n';
    case 0x7F: return '\b';
    default: return c;
    }
}
