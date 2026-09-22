#ifndef TEENSY_TRNG_H
#define TEENSY_TRNG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * The i.MX RT1062 true random number generator samples a free-running ring
 * oscillator against the bus clock and conditions the result with Von
 * Neumann sampling plus statistical self-tests (monobit, run-length,
 * poker, and frequency count) before presenting 512-bit entropy words in
 * ENT0-ENT15.  The NXP-characterized tuning values used here are the same
 * defaults as the official fsl_trng driver.
 */

enum {
  TRNG_OK = 0,
  TRNG_ERROR_NOT_INITIALIZED = -1,
  TRNG_ERROR_TIMEOUT = -2,
  TRNG_ERROR_HW = -3,
  TRNG_ERROR_INVALID = -4,
};

/**
 * Enables the TRNG clock, resets the module to default tuning, applies the
 * NXP-recommended entropy delay and statistical check limits, and starts
 * entropy generation.
 *
 * Entropy generation is slow on the first word: the default sample size of
 * 2500 oscillator samples at the default 3200-clock entropy delay takes on
 * the order of a millisecond per 512-bit word.
 *
 * @return TRNG_OK on success, or a trng error code on failure.
 */
int trng_init(void);

/** Returns whether trng_init() has completed successfully. */
bool trng_is_initialized(void);

/**
 * Fills @p buffer with @p length bytes of true random data.
 *
 * Safe to call from any context that may busy-wait; each 512-bit entropy
 * word costs roughly a millisecond, so callers needing many bytes should
 * budget accordingly (e.g. 64 bytes ~= 1 ms).
 *
 * @return TRNG_OK on success, or a trng error code on failure.
 */
int trng_read(void *buffer, size_t length);

/** Returns one 32-bit random word. */
uint32_t trng_word(void);

#endif
