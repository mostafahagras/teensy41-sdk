#ifndef TEENSY_TIME_H
#define TEENSY_TIME_H

#include <stdint.h>

void time_init(void);
uint32_t time_millis(void);
uint32_t time_micros(void);
void time_delay_ms(uint32_t milliseconds);
void time_delay_us(uint32_t microseconds);

#endif
