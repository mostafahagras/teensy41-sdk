#include <stdbool.h>
#include <stdint.h>

#include <teensy/imxrt.h>
#include <teensy/watchdog.h>

/*
 * RTWDOG register writes are protected by a short write-completion window.
 * This follows the NXP RTWDOG driver sequence:
 *
 *   1. disable interrupts while preserving PRIMASK;
 *   2. write the update key to CNT and wait for ULK;
 *   3. write WIN, TOVAL, then CS, and wait for RCS;
 *   4. restore the caller's interrupt state.
 *
 * CMD32EN is set in CS, so future refreshes use the atomic 32-bit refresh
 * key (0xB480A602).  The unlock and refresh helpers still support a prior
 * updateable configuration that uses 16-bit command writes. The sequence was
 * cross-checked against NXP's fsl_rtwdog driver and the tested WDT_T4 Teensy
 * 4 library. CS, TOVAL, and WIN are write-once for each unlock window; do
 * not write them elsewhere.
 */

#define WATCHDOG_UPDATE_KEY 0xD928C520u
#define WATCHDOG_REFRESH_KEY 0xB480A602u
#define WATCHDOG_PRESCALER_DIVIDER 256u
#define WATCHDOG_TOVAL_MAX UINT16_MAX
#define WATCHDOG_LPO_CLOCK_SELECT 1u
#define WATCHDOG_STATUS_WAIT_LIMIT 1000000u
#define WATCHDOG_CNT16 (*(volatile uint16_t *)(IMXRT_WDOG3_ADDRESS + 4u))

static inline __attribute__((always_inline)) uint32_t
watchdog_save_and_disable_interrupts(void) {
  uint32_t primask;

  __asm volatile("mrs %0, primask\n"
                 "cpsid i"
                 : "=r"(primask)
                 :
                 : "memory");
  return primask;
}

static inline __attribute__((always_inline)) void
watchdog_restore_interrupts(uint32_t primask) {
  __asm volatile("msr primask, %0" : : "r"(primask) : "memory");
}

static inline __attribute__((always_inline)) bool
watchdog_wait_for_status(uint32_t status) {
  uint32_t count;

  for (count = 0u; count < WATCHDOG_STATUS_WAIT_LIMIT; ++count) {
    if (WDOG3_CS & status)
      return true;
  }
  return false;
}

static inline __attribute__((always_inline)) void
watchdog_write_command(uint32_t command) {
  if (WDOG3_CS & WDOG_CS_CMD32EN) {
    WDOG3_CNT = command;
  } else {
    WATCHDOG_CNT16 = (uint16_t)command;
    WATCHDOG_CNT16 = (uint16_t)(command >> 16);
  }
}

static inline __attribute__((always_inline)) bool watchdog_unlock(void) {
  watchdog_write_command(WATCHDOG_UPDATE_KEY);
  return watchdog_wait_for_status(WDOG_CS_ULK);
}

static uint32_t watchdog_timeout_to_ticks(uint32_t timeout_ms,
                                          uint32_t prescaler) {
  const uint64_t divisor = 1000u * prescaler;

  return (uint32_t)(((uint64_t)timeout_ms * WATCHDOG_CLOCK_HZ + divisor - 1u) /
                    divisor);
}

/* Keep every instruction between unlock and CS in this division-free helper. */
static __attribute__((noinline)) int
watchdog_apply_config(uint32_t timeout_ticks, uint32_t control) {
  uint32_t primask = watchdog_save_and_disable_interrupts();

  if (!watchdog_unlock()) {
    watchdog_restore_interrupts(primask);
    return WATCHDOG_ERROR_LOCKED;
  }
  WDOG3_WIN = 0u;
  WDOG3_TOVAL = timeout_ticks;
  WDOG3_CS = control;
  if (!watchdog_wait_for_status(WDOG_CS_RCS)) {
    watchdog_restore_interrupts(primask);
    return WATCHDOG_ERROR_TIMEOUT;
  }
  watchdog_restore_interrupts(primask);

  return WATCHDOG_OK;
}

/* Keep the protected disable sequence similarly free of calls and division. */
static __attribute__((noinline)) int watchdog_apply_disable(void) {
  uint32_t primask = watchdog_save_and_disable_interrupts();

  if (!watchdog_unlock()) {
    watchdog_restore_interrupts(primask);
    return WATCHDOG_ERROR_TIMEOUT;
  }
  WDOG3_CS = (WDOG3_CS & ~WDOG_CS_EN) & ~WDOG_CS_FLG;
  if (!watchdog_wait_for_status(WDOG_CS_RCS)) {
    watchdog_restore_interrupts(primask);
    return WATCHDOG_ERROR_TIMEOUT;
  }
  watchdog_restore_interrupts(primask);

  return WATCHDOG_OK;
}

int watchdog_init(uint32_t timeout_ms) {
  uint32_t prescaler = 1u;
  uint32_t timeout_ticks;
  uint32_t control;

  if (timeout_ms == 0u || timeout_ms > WATCHDOG_MAX_TIMEOUT_MS)
    return WATCHDOG_ERROR_INVALID;

  if (timeout_ms > (WATCHDOG_TOVAL_MAX * 1000u) / WATCHDOG_CLOCK_HZ) {
    prescaler = WATCHDOG_PRESCALER_DIVIDER;
  }
  timeout_ticks = watchdog_timeout_to_ticks(timeout_ms, prescaler);

  CCM_CCGR5 |= CCM_CCGR5_WDOG3(CCM_CCGR_ON);

  /* UPDATE governs reconfiguration after the initial setup.  A disabled
   * watchdog may still be awaiting its first configuration, so let the
   * bounded unlock sequence determine whether that state is writable. */
  if ((WDOG3_CS & WDOG_CS_EN) && !(WDOG3_CS & WDOG_CS_UPDATE))
    return WATCHDOG_ERROR_LOCKED;

  control = WDOG_CS_EN | WDOG_CS_UPDATE | WDOG_CS_CMD32EN | WDOG_CS_WAIT |
            WDOG_CS_CLK(WATCHDOG_LPO_CLOCK_SELECT);
  if (prescaler == WATCHDOG_PRESCALER_DIVIDER)
    control |= WDOG_CS_PRES;

  return watchdog_apply_config(timeout_ticks, control);
}

void watchdog_feed(void) {
  uint32_t primask = watchdog_save_and_disable_interrupts();

  watchdog_write_command(WATCHDOG_REFRESH_KEY);
  watchdog_restore_interrupts(primask);
}

int watchdog_disable(void) {
  if (!watchdog_is_enabled())
    return WATCHDOG_OK;
  if (!(WDOG3_CS & WDOG_CS_UPDATE))
    return WATCHDOG_ERROR_LOCKED;

  return watchdog_apply_disable();
}

bool watchdog_is_enabled(void) { return (WDOG3_CS & WDOG_CS_EN) != 0u; }

bool watchdog_was_reset(void) {
  return (SRC_SRSR & SRC_SRSR_WDOG3_RST_B) != 0u;
}
