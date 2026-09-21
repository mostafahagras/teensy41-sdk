#ifndef TEENSY_TEMPMON_H
#define TEENSY_TEMPMON_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The i.MX RT1062 TEMPMON module measures the die temperature using a
 * factory-calibrated sensor.  Calibration data (hot temperature, hot count,
 * and room count) is fused in OCOTP word ANA1 and read at init, so reported
 * temperatures need no user calibration.
 *
 * TEMPMON runs from the 24 MHz crystal oscillator and keeps measuring
 * autonomously once started; tempmon_get_temp_c() simply waits for the next
 * finished conversion, which is at most one measure period old.
 */

enum {
  TEMPMON_OK = 0,
  TEMPMON_ERROR_CALIBRATION = -1,
  TEMPMON_ERROR_TIMEOUT = -2,
  TEMPMON_ERROR_INVALID = -3,
};

/** Default alarm thresholds in degrees Celsius, matching Teensyduino. */
#define TEMPMON_DEFAULT_HIGH_ALARM_C 85.0f
#define TEMPMON_DEFAULT_LOW_ALARM_C 25.0f
#define TEMPMON_DEFAULT_PANIC_ALARM_C 90.0f

/**
 * Powers up the temperature sensor, loads the OCOTP ANA1 factory calibration,
 * sets the alarm thresholds, and starts periodic measurements.
 *
 * Also routes IRQ_TEMPERATURE_PANIC to a handler that halts the chip, matching
 * Teensyduino's panic-shutdown behavior.  Use tempmon_attach_high_alarm() /
 * tempmon_attach_low_alarm() for ordinary (non-panic) alarms.
 *
 * @return TEMPMON_OK on success, or a tempmon error code on failure.
 */
int tempmon_init(void);

/** Returns the current die temperature in degrees Celsius. */
float tempmon_get_temp_c(void);

/** Returns the current die temperature in degrees Fahrenheit. */
float tempmon_get_temp_f(void);

/**
 * Sets the temperature at which periodic monitoring raises a panic alarm.
 * The panic alarm halts the chip regardless of the ordinary alarm handlers.
 * @return TEMPMON_OK on success, or TEMPMON_ERROR_INVALID if out of range.
 */
int tempmon_set_panic_alarm_c(float degrees_c);

/**
 * Attaches a callback invoked when the temperature rises above the threshold.
 * The alarm fires once per crossing and is rearmed when the temperature
 * falls back below the low alarm threshold.  Passing NULL detaches the
 * callback but leaves the alarm thresholds active.
 */
int tempmon_attach_high_alarm(float degrees_c, void (*callback)(void));

/**
 * Attaches a callback invoked when the temperature falls below the threshold.
 * The alarm fires once per crossing and is rearmed when the temperature
 * rises back above the high alarm threshold.  Passing NULL detaches the
 * callback but leaves the alarm thresholds active.
 */
int tempmon_attach_low_alarm(float degrees_c, void (*callback)(void));

/** Stops periodic measurements without powering down the sensor. */
void tempmon_stop(void);

/** Resumes periodic measurements after tempmon_stop(). */
void tempmon_start(void);

/** Powers down the temperature sensor. */
void tempmon_power_down(void);

/** Shared ISR for high/low alarm crossings; installed by the attach
 * functions.  Declared for completeness; applications should not call it. */
void tempmon_temperature_isr(void);

#endif
