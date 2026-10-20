#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_init(uint32_t frequency);
uint32_t timer_ticks(void);
uint32_t timer_frequency(void);
void timer_sleep(uint32_t ms);

#endif
