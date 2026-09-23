#include <stddef.h>
#include <teensy/boottime.h>
#include <teensy/clock.h>
#include <teensy/imxrt.h>

volatile uint32_t boottime_cycle_switch;
volatile uint32_t boottime_cycles_at_main;
volatile uint32_t boottime_cycles_clock_entry;
volatile uint32_t boottime_cycles_dcdc_done;
volatile uint32_t boottime_cycles_pll_done;

/* The post-switch segment is measured from the point where time_init()
 * re-zeroed the same counter (the DWT has a single register: the segments
 * must not be subtracted across that reset, or the subtraction wraps to
 * ~2^32 and phase B reads ~7.15 s at 600 MHz). */
void boottime_us(uint32_t *before_switch, uint32_t *after_switch) {
  uint32_t switch_cycle = boottime_cycle_switch;
  uint32_t main_cycle = boottime_cycles_at_main;
  uint32_t cpu_mhz = clock_cpu_frequency_hz / 1000000u;
  uint32_t rom_hz = BOOTTIME_ROM_HZ / 1000000u;

  if (before_switch != NULL)
    *before_switch = switch_cycle / rom_hz;
  if (after_switch != NULL)
    *after_switch = cpu_mhz ? main_cycle / cpu_mhz : main_cycle;
}

uint32_t boottime_us_total(void) {
  uint32_t before;
  uint32_t after;

  boottime_us(&before, &after);
  return before + after;
}

/* Wait-portion of clock_init in RAW cycles (the pre-switch CPU rate is
 * whatever the ROM left, so converting to microseconds here would be
 * guesswork): dcdc covers the voltage-settle wait, pll the ARM-PLL lock
 * wait. */
void boottime_clock_waits_cycles(uint32_t *dcdc, uint32_t *pll) {
  if (dcdc != NULL)
    *dcdc = boottime_cycles_dcdc_done - boottime_cycles_clock_entry;
  if (pll != NULL)
    *pll = boottime_cycles_pll_done - boottime_cycles_dcdc_done;
}
