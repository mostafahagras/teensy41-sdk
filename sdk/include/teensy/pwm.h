#ifndef TEENSY_PWM_H
#define TEENSY_PWM_H

#include <stdint.h>

void pwm_init(void);
int pwm_write(uint8_t pin, uint32_t value);
int pwm_set_frequency(uint8_t pin, float frequency_hz);
uint32_t pwm_set_resolution(uint32_t bits);

#endif
