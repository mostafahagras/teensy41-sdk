#ifndef TEENSY_WATCHDOG_H
#define TEENSY_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The RT1062 RTWDOG (WDOG3) uses the nominal 32 kHz internal low-power
 * oscillator selected by watchdog_init().  Its RC tolerance makes wall-clock
 * timeouts approximate.  A 16-bit timeout value and its optional /256
 * prescaler permit nominal timeouts from 1 ms through
 * WATCHDOG_MAX_TIMEOUT_MS.
 */
#define WATCHDOG_CLOCK_HZ 32000u
#define WATCHDOG_MAX_TIMEOUT_MS 524280u

enum {
  WATCHDOG_OK = 0,
  WATCHDOG_ERROR_INVALID = -1,
  WATCHDOG_ERROR_LOCKED = -2,
  WATCHDOG_ERROR_TIMEOUT = -3,
};

/**
 * Starts RTWDOG (WDOG3) with a nominal timeout rounded up to the next LPO tick.
 * The watchdog continues running in chip Wait mode, which the SDK uses for
 * WFI-based delays.  It pauses in Stop and Debug modes; window and pre-timeout
 * interrupt modes are disabled.
 *
 * The watchdog configuration remains updateable, so watchdog_init() may
 * change an already-running watchdog and watchdog_disable() may stop it.  If
 * an already-running watchdog has updates disabled, reconfiguration returns
 * WATCHDOG_ERROR_LOCKED.
 *
 * @return WATCHDOG_OK on success, or a watchdog error code on failure.
 */
int watchdog_init(uint32_t timeout_ms);

/** Refreshes a watchdog started by watchdog_init(). */
void watchdog_feed(void);

/** Stops a watchdog started with an updateable configuration.
 * @return WATCHDOG_OK on success, or a watchdog error code on failure.
 */
int watchdog_disable(void);

/** Returns whether RTWDOG is currently enabled. */
bool watchdog_is_enabled(void);

/** Returns whether the current boot was caused by RTWDOG (WDOG3). */
bool watchdog_was_reset(void);

#endif
