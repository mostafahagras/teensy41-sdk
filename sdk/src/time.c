#include <stdint.h>

#include <teensy/clock.h>
#include <teensy/imxrt.h>
#include <teensy/time.h>

#define SYSTICK_EXTERNAL_HZ 100000u

static volatile uint32_t time_millis_count;
static volatile uint32_t time_cycle_at_tick;

__attribute__((section(".fastrun"))) void time_systick_handler(void) {
  ++time_millis_count;
  time_cycle_at_tick = ARM_DWT_CYCCNT;
}

void time_init(void) {
  ARM_DEMCR |= ARM_DEMCR_TRCENA;
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
  ARM_DWT_CYCCNT = 0;
  time_cycle_at_tick = 0;
  time_millis_count = 0;

  _VectorsRam[15] = time_systick_handler;
  SYST_RVR = (SYSTICK_EXTERNAL_HZ / 1000u) - 1u;
  SYST_CVR = 0;
  SYST_CSR = SYST_CSR_TICKINT | SYST_CSR_ENABLE;
  SCB_SHPR3 = (SCB_SHPR3 & 0x00FFFFFFu) | (32u << 24);
}

uint32_t time_millis(void) { return time_millis_count; }

uint32_t time_micros(void) {
  uint32_t milliseconds;
  uint32_t cycle_at_tick;
  uint32_t cycle_now;

  do {
    milliseconds = time_millis_count;
    cycle_at_tick = time_cycle_at_tick;
    cycle_now = ARM_DWT_CYCCNT;
  } while (milliseconds != time_millis_count);

  return milliseconds * 1000u +
         (uint32_t)(((uint64_t)(cycle_now - cycle_at_tick) * 1000000u) /
                    clock_cpu_frequency_hz);
}

void time_delay_us(uint32_t microseconds) {
  uint32_t start = time_micros();

  while ((uint32_t)(time_micros() - start) < microseconds) {
    __asm volatile("nop");
  }
}

void time_delay_ms(uint32_t milliseconds) {
  uint32_t start = time_millis();

  while ((uint32_t)(time_millis() - start) < milliseconds) {
    __asm volatile("wfi");
  }
}
