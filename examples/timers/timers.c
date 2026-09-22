/* Timer hardware: PIT interval timers + GPT compare alarms.
 *
 * PIT channel 0 ticks at 1 kHz and reports drift against the SysTick-backed
 * time module (expect single-digit ppm); channel 1 toggles the LED at 1 Hz;
 * a one-shot on channel 2 re-arms itself and reports the max inter-tick gap.
 *
 * GPT1 timestamps the loop delay (24 MHz crystal clock), and GPT2 compare
 * 1 + 2 run their own 1 s / 250 ms alarms.
 */

#include <teensy/gpio.h>
#include <teensy/gpt.h>
#include <teensy/printf.h>
#include <teensy/timer.h>
#include <teensy/time.h>
#include <teensy/usb.h>

static volatile uint32_t tick_count;
static volatile uint32_t tick_last_us;
static volatile uint32_t tick_max_gap_us;
static volatile uint32_t oneshot_count;
static volatile uint32_t gpt_alarm_count;
static uint32_t ticks_seen;

static void tick_isr(void) {
  uint32_t now = time_micros();
  uint32_t gap = now - tick_last_us;
  ++tick_count;
  tick_last_us = now;
  if (gap > tick_max_gap_us)
    tick_max_gap_us = gap;
}

static void pit_oneshot_isr(void) {
  ++oneshot_count;
  (void)timer_oneshot(TIMER_PIT2, 1000000u, pit_oneshot_isr, 64);
}

static void pit_blink_isr(void) { gpio_toggle(13); }

static void gpt_alarm_isr(void) { ++gpt_alarm_count; }

int main(void) {
  usb_init();
  while (!usb_connected())
    time_delay_ms(100);
  time_delay_ms(300);
  gpio_configure(13, GPIO_OUTPUT);

  if (timer_periodic(TIMER_PIT0, 1000u, tick_isr, 32) != TIMER_OK ||
      timer_periodic(TIMER_PIT1, 500000u, pit_blink_isr, 64) != TIMER_OK ||
      timer_oneshot(TIMER_PIT2, 1000000u, pit_oneshot_isr, 64) != TIMER_OK) {
    puts("PIT init failed");
    return 1;
  }
  if (gpt_init() != GPT_OK ||
      gpt_attach_compare(GPT_TIMER2, 1, 1000000u, gpt_alarm_isr, 48) != GPT_OK) {
    puts("GPT init failed");
    return 1;
  }

  while (1) {
    uint32_t before = gpt_counter(GPT_TIMER1);
    time_delay_ms(1000);
    uint32_t elapsed = gpt_elapsed_us(GPT_TIMER1, before);
    int64_t drift_ppm = ((int64_t)(tick_count - ticks_seen) - (int64_t)elapsed) *
                        1000000 / (int64_t)elapsed;
    ticks_seen = tick_count;
    printf("ticks=%lu (drift %ld ppm) max_gap=%lu us oneshots=%lu alarms=%lu\r\n",
           (unsigned long)tick_count, (long)drift_ppm,
           (unsigned long)tick_max_gap_us, (unsigned long)oneshot_count,
           (unsigned long)gpt_alarm_count);
    tick_max_gap_us = 0;
  }
}
