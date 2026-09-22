#ifndef TEENSY_RTC_H
#define TEENSY_RTC_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The i.MX RT1062 secure real-time counter keeps wall-clock seconds in the
 * LP SRTC (SNVS), which runs from the always-on VBAT domain and keeps
 * counting through power-off as long as the coin cell or VBAT supply is
 * present.  The HP RTC mirrors the counter and adds 15 bits of 32768 Hz
 * sub-second resolution.
 *
 * All functions take or return Unix seconds (days since 1970-01-01 times
 * 86400 plus seconds of day).  The counter rolls over in 2106.
 */

enum {
  RTC_OK = 0,
  RTC_ERROR_TIMEOUT = -1,
};

/** Date and time broken into fields; matches the TimeLib tmElements_t
 * layout, except year is the full calendar year. */
typedef struct {
  uint8_t second;  /* 0-59 */
  uint8_t minute;  /* 0-59 */
  uint8_t hour;    /* 0-23 */
  uint8_t weekday; /* 1-7, Sunday = 1 */
  uint8_t day;     /* 1-31 */
  uint8_t month;   /* 1-12 */
  uint16_t year;   /* full calendar year, e.g. 2026 */
} rtc_datetime_t;

/**
 * Starts the LP SRTC and HP RTC if the LP counter is not already running
 * (a first boot or a dead VBAT battery).  An already-running counter is
 * left untouched, so a time set in a previous run survives a re-flash.
 *
 * Called by startup; calling it again is harmless.
 */
void rtc_init(void);

/** Returns whether the LP SRTC is running and holds a valid time. */
bool rtc_is_valid(void);

/** Reads the current Unix time in seconds. */
uint32_t rtc_get(void);

/**
 * Reads the current Unix time with sub-second resolution.
 * @param milliseconds Set to the millisecond part of the current second.
 * @return The current Unix time in seconds.
 */
uint32_t rtc_get_ms(uint32_t *milliseconds);

/** Writes the Unix time and restarts both counters. */
void rtc_set(uint32_t seconds);

/** Reads the current time into @p datetime.
 * @return RTC_OK on success, or RTC_ERROR_TIMEOUT on failure.
 */
int rtc_get_datetime(rtc_datetime_t *datetime);

/**
 * Writes @p datetime to the counter.
 * @return RTC_OK on success, or RTC_ERROR_INVALID for out-of-range fields.
 */
int rtc_set_datetime(const rtc_datetime_t *datetime);

/** Converts broken-down time to Unix seconds. */
uint32_t rtc_make_time(const rtc_datetime_t *datetime);

/** Converts Unix seconds to broken-down time. */
void rtc_break_time(uint32_t seconds, rtc_datetime_t *datetime);

#endif
