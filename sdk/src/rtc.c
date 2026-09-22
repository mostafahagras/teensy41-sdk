#include <teensy/imxrt.h>
#include <teensy/rtc.h>

#include <stddef.h>

/*
 * SNVS clock layout.  The LP SRTC counts 32768 Hz ticks; LPSRTCMR holds the
 * upper 15 bits of the 32-bit seconds counter and LPSRTCLR the lower 17
 * bits plus a 15-bit sub-second prescale.  Seconds are therefore
 * {CMR[14:0], CLR[31:15]}, and the HP mirror registers expose the same
 * value in the same layout.
 */
#define SNVS_SRTC_SECONDS_SHIFT 15
#define SNVS_SRTC_SECONDS_LOW_BITS 17
#define SNVS_SRTC_SECONDS_LOW_MASK 0x1FFFFu

/* Default epoch on first boot: Saturday, January 1, 2022 00:00:00 UTC.
 * (Teensyduino uses 2019-01-01.) */
#define RTC_DEFAULT_EPOCH 1640995200u

#define RTC_WAIT_LIMIT 1000000u

#define RTC_DAYS_IN_YEAR 365u
static const uint8_t rtc_month_days[12] = {31, 28, 31, 30, 31, 30,
                                           31, 31, 30, 31, 30, 31};

static bool rtc_leap_year(uint16_t year) {
  return (year % 4u) == 0u && ((year % 100u) != 0u || (year % 400u) == 0u);
}

static uint16_t rtc_days_in_year(uint16_t year) {
  return rtc_leap_year(year) ? RTC_DAYS_IN_YEAR + 1u : RTC_DAYS_IN_YEAR;
}

static uint8_t rtc_days_in_month(uint16_t year, uint8_t month) {
  if (month == 2u && rtc_leap_year(year))
    return 29u;
  return rtc_month_days[month - 1u];
}

/* Reads the 32-bit seconds counter coherently: the prescale ticks at
 * 32768 Hz, so a carry can race between the two register reads.  Retry
 * until both reads agree (same approach as Teensyduino rtc_get). */
static uint32_t rtc_read_seconds(void) {
  uint32_t hi1 = SNVS_LPSRTCMR;
  uint32_t lo1 = SNVS_LPSRTCLR;

  for (;;) {
    uint32_t hi2 = SNVS_LPSRTCMR;
    uint32_t lo2 = SNVS_LPSRTCLR;
    if (hi1 == hi2 && lo1 == lo2)
      return (hi2 << (32u - SNVS_SRTC_SECONDS_SHIFT)) |
             ((lo2 >> SNVS_SRTC_SECONDS_SHIFT) & SNVS_SRTC_SECONDS_LOW_MASK);
    hi1 = hi2;
    lo1 = lo2;
  }
}

void rtc_init(void) {
  /* Exact Teensyduino startup sequence: if the SRTC is not running, load
   * the default epoch and start it.  Nothing else - no clock gating, no
   * HPCOMR, no LPPGDR writes - because extra LP-domain writes from the ARM
   * side can disturb security state on the RT1062. */
  if ((SNVS_LPCR & SNVS_LPCR_SRTC_ENV) == 0u) {
    SNVS_LPSRTCMR = RTC_DEFAULT_EPOCH >> SNVS_SRTC_SECONDS_LOW_BITS;
    SNVS_LPSRTCLR = RTC_DEFAULT_EPOCH << SNVS_SRTC_SECONDS_SHIFT;
    SNVS_LPCR |= SNVS_LPCR_SRTC_ENV;
  }
  /* Start the HP counter and sync it to the SRTC.  The HP prescale counts
   * 32768 Hz ticks and supplies the sub-second field of rtc_get_ms(). */
  SNVS_HPCR |= SNVS_HPCR_RTC_EN | SNVS_HPCR_HP_TS;
}

bool rtc_is_valid(void) { return (SNVS_LPCR & SNVS_LPCR_SRTC_ENV) != 0u; }

uint32_t rtc_get(void) { return rtc_read_seconds(); }

uint32_t rtc_get_ms(uint32_t *milliseconds) {
  uint32_t hi1 = SNVS_HPRTCMR;
  uint32_t lo1 = SNVS_HPRTCLR;

  for (;;) {
    uint32_t hi2 = SNVS_HPRTCMR;
    uint32_t lo2 = SNVS_HPRTCLR;
    if (hi1 == hi2 && lo1 == lo2) {
      uint32_t prescale = lo2 & (SNVS_SRTC_SECONDS_LOW_MASK >> 2);
      if (milliseconds != NULL)
        *milliseconds = (prescale * 1000u) >> 15;
      return (hi2 << (32u - SNVS_SRTC_SECONDS_SHIFT)) |
             ((lo2 >> SNVS_SRTC_SECONDS_SHIFT) & SNVS_SRTC_SECONDS_LOW_MASK);
    }
    hi1 = hi2;
    lo1 = lo2;
  }
}

void rtc_set(uint32_t seconds) {
  uint32_t wait = RTC_WAIT_LIMIT;

  /* Stop both counters before writing, as the reference manual requires. */
  SNVS_HPCR &= ~(SNVS_HPCR_RTC_EN | SNVS_HPCR_HP_TS);
  while ((SNVS_HPCR & SNVS_HPCR_RTC_EN) != 0u && wait-- != 0u)
    __asm volatile("nop");

  wait = RTC_WAIT_LIMIT;
  SNVS_LPCR &= ~SNVS_LPCR_SRTC_ENV;
  while ((SNVS_LPCR & SNVS_LPCR_SRTC_ENV) != 0u && wait-- != 0u)
    __asm volatile("nop");

  SNVS_LPSRTCMR = seconds >> SNVS_SRTC_SECONDS_LOW_BITS;
  SNVS_LPSRTCLR = seconds << SNVS_SRTC_SECONDS_SHIFT;

  SNVS_LPCR |= SNVS_LPCR_SRTC_ENV;
  while ((SNVS_LPCR & SNVS_LPCR_SRTC_ENV) == 0u && wait-- != 0u)
    __asm volatile("nop");

  /* Restart the HP counter and sync it to the LP counter. */
  SNVS_HPCR |= SNVS_HPCR_RTC_EN | SNVS_HPCR_HP_TS;
}

uint32_t rtc_make_time(const rtc_datetime_t *datetime) {
  uint32_t seconds;
  uint16_t year;
  uint8_t month;

  if (datetime == NULL || datetime->month < 1u || datetime->month > 12u ||
      datetime->day < 1u || datetime->day > 31u || datetime->hour > 23u ||
      datetime->minute > 59u || datetime->second > 59u)
    return 0;

  seconds = 0;
  for (year = 1970u; year < datetime->year; ++year)
    seconds += rtc_days_in_year(year);
  for (month = 1u; month < datetime->month; ++month)
    seconds += rtc_days_in_month(datetime->year, month);
  seconds = seconds * 86400u + (uint32_t)(datetime->day - 1u) * 86400u +
            (uint32_t)datetime->hour * 3600u +
            (uint32_t)datetime->minute * 60u + datetime->second;
  return seconds;
}

void rtc_break_time(uint32_t seconds, rtc_datetime_t *datetime) {
  uint32_t days;
  uint16_t year;
  uint8_t month;

  if (datetime == NULL)
    return;

  datetime->second = (uint8_t)(seconds % 60u);
  seconds /= 60u;
  datetime->minute = (uint8_t)(seconds % 60u);
  seconds /= 60u;
  datetime->hour = (uint8_t)(seconds % 24u);
  seconds /= 24u;

  days = seconds;
  datetime->weekday = (uint8_t)(((days + 4u) % 7u) + 1u); /* 1970-01-01 = Thu */

  year = 1970u;
  while (days >= rtc_days_in_year(year)) {
    days -= rtc_days_in_year(year);
    ++year;
  }
  datetime->year = year;

  month = 1u;
  while (days >= rtc_days_in_month(year, month)) {
    days -= rtc_days_in_month(year, month);
    ++month;
  }
  datetime->month = month;
  datetime->day = (uint8_t)(days + 1u);
}

int rtc_get_datetime(rtc_datetime_t *datetime) {
  uint32_t seconds;

  if (datetime == NULL || !rtc_is_valid())
    return RTC_ERROR_TIMEOUT;
  seconds = rtc_read_seconds();
  rtc_break_time(seconds, datetime);
  return RTC_OK;
}

int rtc_set_datetime(const rtc_datetime_t *datetime) {
  uint32_t seconds;

  if (datetime == NULL)
    return RTC_ERROR_TIMEOUT;
  seconds = rtc_make_time(datetime);
  if (seconds == 0u)
    return RTC_ERROR_TIMEOUT;
  rtc_set(seconds);
  return RTC_OK;
}
