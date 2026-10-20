#include "keyboard.h"

#include <stdint.h>

#include "idt.h"
#include "io.h"
#include "kstring.h"
#include "pic.h"

#define KBD_DATA_PORT 0x60
#define BUFFER_SIZE   128

// Scan codes (set 1) of the modifier keys
#define SC_EXTENDED    0xE0
#define SC_RELEASED    0x80
#define SC_LEFT_SHIFT  0x2A
#define SC_RIGHT_SHIFT 0x36
#define SC_CTRL        0x1D
#define SC_ALT         0x38
#define SC_CAPS_LOCK   0x3A

// Characters >= 0x80 are code page 437 glyphs, the VGA text mode font
#define CP437_E_ACUTE  0x82
#define CP437_A_GRAVE  0x85
#define CP437_C_CEDIL  0x87
#define CP437_E_GRAVE  0x8A
#define CP437_U_GRAVE  0x97
#define CP437_POUND    0x9C
#define CP437_MICRO    0xE6
#define CP437_DEGREE   0xF8
#define CP437_SQUARED  0xFD
#define CP437_SECTION  0x15

// Numeric keypad, identical in every layout
#define KEYPAD_KEYS \
    [0x37] = '*', [0x47] = '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'

struct keymap {
    const char *name;
    const uint8_t *normal;
    const uint8_t *shift;
    const uint8_t *altgr;
};

static const uint8_t us_normal[128] = {
    [0x01] = 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    [0x1E] = 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    [0x2B] = '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    [0x39] = ' ', [0x56] = '\\', KEYPAD_KEYS,
};

static const uint8_t us_shift[128] = {
    [0x01] = 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    [0x1E] = 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    [0x2B] = '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    [0x39] = ' ', [0x56] = '|', KEYPAD_KEYS,
};

static const uint8_t fr_normal[128] = {
    [0x01] = 27, '&', CP437_E_ACUTE, '"', '\'', '(', '-', CP437_E_GRAVE, '_',
    CP437_C_CEDIL, CP437_A_GRAVE, ')', '=', '\b',
    '\t', 'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '^', '$', '\n',
    [0x1E] = 'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', CP437_U_GRAVE, CP437_SQUARED,
    [0x2B] = '*', 'w', 'x', 'c', 'v', 'b', 'n', ',', ';', ':', '!',
    [0x39] = ' ', [0x56] = '<', KEYPAD_KEYS,
};

static const uint8_t fr_shift[128] = {
    [0x01] = 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', CP437_DEGREE, '+', '\b',
    '\t', 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', [0x1B] = CP437_POUND, '\n',
    [0x1E] = 'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '%',
    [0x2B] = CP437_MICRO, 'W', 'X', 'C', 'V', 'B', 'N', '?', '.', '/', CP437_SECTION,
    [0x39] = ' ', [0x56] = '>', KEYPAD_KEYS,
};

static const uint8_t fr_altgr[128] = {
    [0x03] = '~', '#', '{', '[', '|', '`', '\\', '^', '@', ']', '}',
};

static const struct keymap keymaps[] = {
    { "fr", fr_normal, fr_shift, fr_altgr },
    { "us", us_normal, us_shift, NULL },
};

// QEMU forwards physical key positions, so the layout must match the host keyboard
static const struct keymap *keymap = &keymaps[0];

static volatile int buffer[BUFFER_SIZE];
static volatile uint32_t buffer_head;  // written by the IRQ handler
static volatile uint32_t buffer_tail;  // read by keyboard_poll

static bool left_shift, right_shift, ctrl, alt, altgr, caps_lock, extended;

static void buffer_push(int key) {
    uint32_t next = (buffer_head + 1) % BUFFER_SIZE;
    if (next == buffer_tail) return;  // full: drop the key
    buffer[buffer_head] = key;
    buffer_head = next;
}

static int translate(uint8_t scancode) {
    uint8_t base = keymap->normal[scancode];
    uint8_t c;

    if (altgr) {
        c = keymap->altgr ? keymap->altgr[scancode] : 0;
    } else {
        bool shifted = left_shift || right_shift;
        if (caps_lock && base >= 'a' && base <= 'z') shifted = !shifted;
        c = shifted ? keymap->shift[scancode] : base;
    }

    // Ctrl+letter gives the matching control character (Ctrl+C = 3, Ctrl+L = 12...)
    if (ctrl && !altgr) {
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 1;
    }
    return c;
}

static void handle_extended(uint8_t scancode, bool released) {
    switch (scancode) {
    case SC_CTRL: ctrl = !released; return;
    case SC_ALT: altgr = !released; return;
    }
    if (released) return;

    switch (scancode) {
    case 0x48: buffer_push(KEY_UP); break;
    case 0x50: buffer_push(KEY_DOWN); break;
    case 0x4B: buffer_push(KEY_LEFT); break;
    case 0x4D: buffer_push(KEY_RIGHT); break;
    case 0x47: buffer_push(KEY_HOME); break;
    case 0x4F: buffer_push(KEY_END); break;
    case 0x53: buffer_push(KEY_DELETE); break;
    case 0x1C: buffer_push('\n'); break;  // keypad Enter
    case 0x35: buffer_push('/'); break;   // keypad slash
    }
}

static void keyboard_callback(struct registers *regs) {
    (void) regs;
    uint8_t scancode = inb(KBD_DATA_PORT);

    if (scancode == SC_EXTENDED) {
        extended = true;
        return;
    }

    bool released = scancode & SC_RELEASED;
    scancode &= ~SC_RELEASED;

    if (extended) {
        extended = false;
        handle_extended(scancode, released);
        return;
    }

    switch (scancode) {
    case SC_LEFT_SHIFT: left_shift = !released; return;
    case SC_RIGHT_SHIFT: right_shift = !released; return;
    case SC_CTRL: ctrl = !released; return;
    case SC_ALT: alt = !released; return;
    case SC_CAPS_LOCK:
        if (!released) caps_lock = !caps_lock;
        return;
    }

    if (released) return;
    int key = translate(scancode);
    if (key) buffer_push(key);
}

void keyboard_init(void) {
    // Discard a key that may still be waiting from the boot process
    while (inb(0x64) & 1) inb(KBD_DATA_PORT);

    register_interrupt_handler(IRQ_KEYBOARD, keyboard_callback);
    pic_unmask(IRQ_KEYBOARD - IRQ_BASE);
}

int keyboard_poll(void) {
    if (buffer_tail == buffer_head) return -1;
    int key = buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
    return key;
}

bool keyboard_set_layout(const char *name) {
    for (size_t i = 0; i < sizeof(keymaps) / sizeof(keymaps[0]); i++) {
        if (strcmp(keymaps[i].name, name) == 0) {
            keymap = &keymaps[i];
            return true;
        }
    }
    return false;
}

const char *keyboard_layout(void) {
    return keymap->name;
}

const char *keyboard_layouts(void) {
    return "fr us";
}
