#ifndef TEENSY_BOOTTIME_H
#define TEENSY_BOOTTIME_H

#include <stdint.h>

/*
 * Startup timing instrumentation, cycle-accurate per phase.
 *
 * The DWT cycle counter is zeroed as the very first step of
 * reset_handler, making it the earliest timestamp the compiled image can
 * observe (the ROM's own few hundred cycles before it are a small,
 * constant, uninstrumented bias).
 *
 * clock_init() retargets the CPU from the ROM's 24 MHz to the requested
 * frequency mid-startup, so raw cycle counts mix two tick rates.  The
 * globals below record that switch so main can convert each segment with
 * its own clock:
 *
 *   phase A (before the switch, 24 MHz): cycle_switch / 24
 *   phase B (target clock): (cycles_at_main - cycle_switch) / divide
 *
 * where the divider is the post-switch megahertz value (600 at default
 * F_CPU; see boottime_us_below() in the SDK for the full split).
 *
 * Cycle counts come from a single register read, so they are exact for
 * their phase and consistent across boots.
 */

/* CPU frequency, in hertz, that runs the whole startup before clock_init
 * retargets it (the ROM leaves the chip on OSC24M, 24 MHz). */
#define BOOTTIME_ROM_HZ 24000000u

/* Cycle count at the clock switch (0 until clock_init completes). */
extern volatile uint32_t boottime_cycle_switch;

/* Cycle counts: at reset_handler's cache_init boundary, and after ARM-PLL
 * lock inside clock_init.  The DCDC settle itself runs in reset_handler;
 * its duration is attributed by these two rows (entry->pll minus the
 * non-waiting config work). */
extern volatile uint32_t boottime_cycles_clock_entry;
extern volatile uint32_t boottime_cycles_pll_done;

/* Cycle count at the very end of startup, just before main() runs. */
extern volatile uint32_t boottime_cycles_at_main;

/** Fills in the two startup phases in microseconds. */
void boottime_us(uint32_t *before_switch, uint32_t *after_switch);

/** Returns total startup time in microseconds, both phases combined. */
uint32_t boottime_us_total(void);

/** Fills in the clock_init wait times (DCDC settle, PLL lock) in
 * microseconds; measured inside the 24 MHz pre-switch segment. */
void boottime_clock_waits_cycles(uint32_t *dcdc, uint32_t *pll);

#endif
