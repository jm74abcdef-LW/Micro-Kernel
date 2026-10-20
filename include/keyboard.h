#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>

// Non-character keys, kept above the 0-255 range used by printable characters.
#define KEY_UP     0x100
#define KEY_DOWN   0x101
#define KEY_LEFT   0x102
#define KEY_RIGHT  0x103
#define KEY_HOME   0x104
#define KEY_END    0x105
#define KEY_DELETE 0x106

void keyboard_init(void);

// Returns the next key from the buffer (a CP437 character or a KEY_* code), or -1.
int keyboard_poll(void);

bool keyboard_set_layout(const char *name);
const char *keyboard_layout(void);
const char *keyboard_layouts(void);

#endif
