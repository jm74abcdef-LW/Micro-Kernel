#include "timer.h"

#include "idt.h"
#include "io.h"
#include "pic.h"

// The Programmable Interval Timer (8253/8254) runs at 1.193182 MHz and fires
// IRQ 0 every time its 16-bit divisor counts down to zero.
#define PIT_BASE_FREQUENCY 1193182
#define PIT_CHANNEL0       0x40
#define PIT_COMMAND        0x43
#define PIT_SQUARE_WAVE    0x36  // channel 0, lobyte/hibyte, mode 3

static volatile uint32_t ticks;
static uint32_t frequency;

static void timer_callback(struct registers *regs) {
    (void) regs;
    ticks++;
}

void timer_init(uint32_t hz) {
    frequency = hz;
    uint32_t divisor = PIT_BASE_FREQUENCY / hz;

    register_interrupt_handler(IRQ_TIMER, timer_callback);
    outb(PIT_COMMAND, PIT_SQUARE_WAVE);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
    pic_unmask(IRQ_TIMER - IRQ_BASE);
}

uint32_t timer_ticks(void) {
    return ticks;
}

uint32_t timer_frequency(void) {
    return frequency;
}

void timer_sleep(uint32_t ms) {
    uint32_t target = ticks + (ms * frequency + 999) / 1000;
    // Signed difference keeps working when the counter wraps around
    while ((int32_t) (ticks - target) < 0) cpu_halt();
}
