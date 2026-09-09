#ifndef TEENSY_CLOCK_H
#define TEENSY_CLOCK_H

#include <stdint.h>

extern volatile uint32_t clock_cpu_frequency_hz;
extern volatile uint32_t clock_bus_frequency_hz;
extern volatile uint32_t clock_uart_frequency_hz;

/** Configures the CPU and peripheral clock tree for a target CPU frequency.
 * @return The CPU frequency actually selected in hertz.
 */
uint32_t clock_init(uint32_t frequency_hz);

#endif
