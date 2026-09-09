#ifndef TEENSY_PWM_H
#define TEENSY_PWM_H

#include <stdint.h>

/** Initializes all PWM controller clocks and channels. */
void pwm_init(void);

/** Sets a PWM pin's duty value using the current resolution.
 *
 * Values above the current resolution's maximum are clamped.
 *
 * @return 0 on success, or -1 if the pin does not support PWM.
 */
int pwm_write(uint8_t pin, uint32_t value);

/** Sets the PWM frequency for the timer associated with a pin.
 *
 * Other pins on the same hardware timer may also be affected.
 *
 * @return 0 on success, or -1 if the pin or frequency is invalid.
 */
int pwm_set_frequency(uint8_t pin, float frequency_hz);

/** Sets the duty-cycle resolution in bits.
 *
 * The requested value is clamped to the supported range of 1 to 16 bits.
 *
 * @return The previous resolution in bits.
 */
uint32_t pwm_set_resolution(uint32_t bits);

#endif
