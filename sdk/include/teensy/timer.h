#ifndef TEENSY_TIMER_H
#define TEENSY_TIMER_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Interval timers on the four PIT channels, the i.MX RT1062 equivalent of
 * Teensyduino's IntervalTimer.  Each channel is a 32-bit down counter with
 * automatic reload; the four channels share one interrupt with a
 * per-channel dispatch table.
 *
 * The RT1062 PIT counter runs from ipg_perclk, which clock_init() routes
 * to the 24 MHz crystal oscillator.  The longest representable period is
 * about 179 seconds at that rate; the driver measures the active mux
 * before converting microseconds, so it keeps working if clock_init
 * changes the PERCLK source.
 */

/** Which PIT channel to use; one callback per channel. */
typedef enum {
  TIMER_PIT0 = 0,
  TIMER_PIT1,
  TIMER_PIT2,
  TIMER_PIT3,
} timer_channel_t;

enum {
  TIMER_OK = 0,
  TIMER_ERROR_CHANNEL = -1, /* channel number out of range            */
  TIMER_ERROR_BUSY = -2,    /* channel already running; stop it first */
  TIMER_ERROR_RANGE = -3,   /* period/delay outside representable range */
};

/** Default NVIC priority for timer callbacks (lower = more urgent). */
#define TIMER_DEFAULT_PRIORITY 32

/**
 * Runs @p callback every @p period_us microseconds, starting immediately.
 * Callbacks run in interrupt context; keep them short.
 *
 * @return TIMER_OK on success, or a timer error code on failure.
 */
int timer_periodic(timer_channel_t channel, uint32_t period_us,
                   void (*callback)(void), uint8_t priority);

/**
 * Runs @p callback once, @p delay_us microseconds from now, then frees the
 * channel automatically.
 *
 * @return TIMER_OK on success, or a timer error code on failure.
 */
int timer_oneshot(timer_channel_t channel, uint32_t delay_us,
                  void (*callback)(void), uint8_t priority);

/** Stops the timer on @p channel and frees it.  Safe if already stopped. */
void timer_stop(timer_channel_t channel);

/** Returns whether @p channel has a timer running. */
bool timer_running(timer_channel_t channel);

#endif
