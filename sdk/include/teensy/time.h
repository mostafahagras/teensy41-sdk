#ifndef TEENSY_TIME_H
#define TEENSY_TIME_H

#include <stdint.h>

/** Initializes the SDK's millisecond and microsecond time bases. */
void time_init(void);

/** Returns milliseconds elapsed since startup, wrapping at UINT32_MAX. */
uint32_t time_millis(void);

/** Returns microseconds elapsed since startup, wrapping at UINT32_MAX. */
uint32_t time_micros(void);

/** Blocks for at least the requested number of milliseconds. */
void time_delay_ms(uint32_t milliseconds);

/** Blocks for at least the requested number of microseconds. */
void time_delay_us(uint32_t microseconds);

#endif
