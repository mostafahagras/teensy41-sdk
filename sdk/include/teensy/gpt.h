#ifndef TEENSY_GPT_H
#define TEENSY_GPT_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The two General Purpose Timers (GPT1/GPT2) are free-running 32-bit
 * counters from the 24 MHz crystal oscillator, kept out of the main clock
 * tree on purpose so timestamps stay valid across CPU speed changes.
 *
 * Two uses:
 *  - Timestamping: gpt_read() returns raw 24 MHz ticks; gpt_elapsed_us()
 *    converts a delta between two reads to microseconds (counts up over
 *    2^32/24e6 = 178.9 s before the rollover, which this API handles).
 *  - Output compare alarms: gpt_attach_compare() arms one of the three
 *    compare channels to a callback every period; the ISR re-arms itself
 *    from the live counter, so beats stay aligned even if a callback runs
 *    late.  Passing NULL detaches.
 */

typedef enum {
  GPT_TIMER1 = 0,
  GPT_TIMER2,
} gpt_timer_t;

/** The GPT count clock in hertz; the CCM PERCLK mux (which clock_init()
 * routes to the 24 MHz crystal) selects this alongside the PIT's clock.
 * The driver reads the mux live rather than trusting a constant. */
#define GPT_TICK_HZ 24000000u

enum {
  GPT_OK = 0,
  GPT_ERROR_TIMER = -1,   /* unknown GPT                    */
  GPT_ERROR_COMPARE = -2, /* unknown compare channel (1-3)  */
  GPT_ERROR_RANGE = -3,   /* period/site outside representable range */
  GPT_ERROR_BUSY = -4,    /* compare channel already attached */
};

/**
 * Powers up and starts both GPT counters in free-run mode.  Safe to call
 * again; running counter values are preserved.
 *
 * @return GPT_OK on success, or a gpt error code on failure.
 */
int gpt_init(void);

/** Stops both counters; values stay readable until the next gpt_init(). */
void gpt_stop(void);

/** Returns the raw 24 MHz counter of the given GPT. */
uint32_t gpt_counter(gpt_timer_t timer);

/**
 * Returns microseconds elapsed between two counter readings.  Wraparound
 * is handled by the modular subtraction, so spans up to the ~179 s
 * wraparound window are exact; longer spans report a small garbage number
 * and callers must not treat the result as valid past that window.
 */
uint32_t gpt_elapsed_us(gpt_timer_t timer, uint32_t ticks_then);

/**
 * Arms output compare channel 1..3 of a GPT to fire @p callback every
 * @p period_us microseconds.  The ISR re-arms itself; pass NULL for the
 * callback to detach.  One callback per compare channel.
 *
 * @return GPT_OK on success, or a gpt error code on failure.
 */
int gpt_attach_compare(gpt_timer_t timer, uint8_t compare, uint32_t period_us,
                       void (*callback)(void), uint8_t priority);

/** Returns whether @p compare channel of @p timer has a callback attached. */
bool gpt_attached(gpt_timer_t timer, uint8_t compare);

#endif
