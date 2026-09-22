#include <teensy/imxrt.h>
#include <teensy/trng.h>

#include <stddef.h>

/*
 * TRNG tuning values.  These match the defaults of NXP's fsl_trng driver
 * for the RT1050/RT106x family: raw sampling into both the entropy shifter
 * and the statistical checker, no ring oscillator division, and the
 * NXP-characterized entropy delay of 3200 bus clocks per sample.
 */
#define TRNG_SAMPLE_MODE_RAW 1u
#define TRNG_OSC_DIV_NONE 0u
#define TRNG_ENT_COUNT 16u

#define TRNG_WAIT_LIMIT 100000000u

static bool trng_ready;

/* Reads entropy register @p index.  Reading ENT15 clears ENT_VAL and starts
 * the next 512-bit generation; a defect workaround requires a dummy read of
 * ENT0 afterwards when draining word 15 so ENT_VAL re-arms cleanly. */
static uint32_t trng_read_entropy(uint32_t index) {
  uint32_t data;

  index %= TRNG_ENT_COUNT;
  data = (&TRNG_ENT0)[index];
  if (index == (TRNG_ENT_COUNT - 1u))
    (void)TRNG_ENT0;
  return data;
}

/* Waits for the current entropy word to become valid, clearing hardware
 * errors.  @return false on timeout or repeated hardware failure. */
static bool trng_wait_valid(void) {
  uint32_t wait = TRNG_WAIT_LIMIT;

  while (wait-- != 0u) {
    uint32_t mctl = TRNG_MCTL;
    if ((mctl & TRNG_MCTL_ENT_VAL) != 0u)
      return true;
    if ((mctl & TRNG_MCTL_ERR) != 0u)
      TRNG_MCTL |= TRNG_MCTL_ERR; /* W1C: clear and let generation retry */
  }
  return false;
}

int trng_init(void) {
  /* Statistical check limits (maximum; minimum = maximum - range). */
  static const struct {
    volatile uint32_t *limit;
    uint32_t max;
    uint32_t range;
  } stat_checks[] = {
      {&TRNG_SCML, TRNG_DEFAULT_MONOBIT_MAXIMUM, 268},
      {&TRNG_SCR1L, TRNG_DEFAULT_RUNBIT1_MAXIMUM, 178},
      {&TRNG_SCR2L, TRNG_DEFAULT_RUNBIT2_MAXIMUM, 122},
      {&TRNG_SCR3L, TRNG_DEFAULT_RUNBIT3_MAXIMUM, 88},
      {&TRNG_SCR4L, TRNG_DEFAULT_RUNBIT4_MAXIMUM, 64},
      {&TRNG_SCR5L, TRNG_DEFAULT_RUNBIT5_MAXIMUM, 46},
      {&TRNG_SCR6PL, TRNG_DEFAULT_RUNBIT6PLUS_MAXIMUM, 46},
  };
  uint32_t i;

  CCM_CCGR6 |= CCM_CCGR6_TRNG(CCM_CCGR_ON);

  /* Enter program mode and reset every tuning register to chip defaults. */
  TRNG_MCTL = TRNG_MCTL_PRGM;
  TRNG_MCTL |= TRNG_MCTL_RST_DEF;

  /* Statistical check limits; each "L" limit register packs its maximum
   * into the low field and its range (max - min) into the high field, and
   * aliases its "C" counter readback register while PRGM is set. */
  for (i = 0; i < sizeof(stat_checks) / sizeof(stat_checks[0]); ++i)
    *stat_checks[i].limit = stat_checks[i].max | (stat_checks[i].range << 16);
  /* Poker test: maximum in PKRMAX[23:0], range in PKRRNG[15:0]. */
  TRNG_PKRMAX = TRNG_DEFAULT_POKER_MAXIMUM;
  TRNG_PKRRNG = (TRNG_DEFAULT_POKER_MAXIMUM - TRNG_DEFAULT_POKER_MINIMUM);
  /* Frequency count: minimum and maximum sample-rate bounds. */
  TRNG_FRQMIN = TRNG_DEFAULT_FREQUENCY_MINIMUM;
  TRNG_FRQMAX = TRNG_DEFAULT_FREQUENCY_MAXIMUM;

  TRNG_SCMISC = TRNG_SCMISC_RTY_CT(TRNG_DEFAULT_RETRY_COUNT) |
                TRNG_SCMISC_LRUN_MAX(TRNG_DEFAULT_RUN_MAX_LIMIT);
  TRNG_SDCTL = TRNG_SDCTL_ENT_DLY(TRNG_DEFAULT_ENTROPY_DELAY) |
               TRNG_SDCTL_SAMP_SIZE(TRNG_DEFAULT_SAMPLE_SIZE);
  TRNG_SBLIM = TRNG_DEFAULT_SPARSE_BIT_LIMIT;

  /* Leave program mode and enable TRNG access so entropy words are
   * generated into ENT0-ENT15. */
  TRNG_MCTL = TRNG_MCTL_SAMP_MODE(TRNG_SAMPLE_MODE_RAW) |
              TRNG_MCTL_OSC_DIV(TRNG_OSC_DIV_NONE);
  TRNG_MCTL |= TRNG_MCTL_TRNG_ACC;

  /* Drain the first word so a fresh generation starts immediately. */
  (void)trng_read_entropy(TRNG_ENT_COUNT - 1u);

  trng_ready = true;
  return TRNG_OK;
}

bool trng_is_initialized(void) { return trng_ready; }

int trng_read(void *buffer, size_t length) {
  uint8_t *out = buffer;
  uint32_t index = 0;

  if (buffer == NULL || length == 0u)
    return TRNG_ERROR_INVALID;
  if (!trng_ready)
    return TRNG_ERROR_NOT_INITIALIZED;

  while (length > 0u) {
    uint32_t word;
    uint32_t chunk;

    if (!trng_wait_valid())
      return TRNG_ERROR_TIMEOUT;
    if ((TRNG_MCTL & TRNG_MCTL_ERR) != 0u) {
      TRNG_MCTL |= TRNG_MCTL_ERR;
      return TRNG_ERROR_HW;
    }

    word = trng_read_entropy(index++);
    chunk = length < 4u ? (uint32_t)length : 4u;
    for (uint32_t i = 0; i < chunk; ++i)
      *out++ = (uint8_t)(word >> (8u * i));
    length -= chunk;
  }

  /* If the last read did not fall on ENT15, drain the current word so the
   * next call sees a fresh generation rather than stale data. */
  if ((index % TRNG_ENT_COUNT) != (TRNG_ENT_COUNT - 1u) % TRNG_ENT_COUNT)
    (void)trng_read_entropy(TRNG_ENT_COUNT - 1u);
  return TRNG_OK;
}

uint32_t trng_word(void) {
  uint32_t word;

  if (trng_read(&word, sizeof(word)) != TRNG_OK)
    return 0;
  return word;
}
