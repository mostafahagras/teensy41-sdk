#ifndef TEENSY_PWM_H
#define TEENSY_PWM_H

#include <stdint.h>

#include <teensy/gpio.h>

typedef struct {
  uint8_t type;
  uint8_t module;
  uint8_t channel;
  uint8_t muxval;
} teensy_pwm_pin_info_t;

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

/* Internal descriptor entry points used by compile-time pin wrappers. */
int pwm_write_info(const teensy_pwm_pin_info_t *info,
                   const teensy_gpio_pin_t *gpio, uint32_t value);
int pwm_set_frequency_info(const teensy_pwm_pin_info_t *info,
                           const teensy_gpio_pin_t *gpio, float frequency_hz);

static inline __attribute__((always_inline)) teensy_pwm_pin_info_t
teensy_pwm_pin_const(uint8_t pin) {
  switch (pin) {
  case 0:
    return (teensy_pwm_pin_info_t){1, 1, 0, 4};
  case 1:
    return (teensy_pwm_pin_info_t){1, 0, 0, 4};
  case 2:
    return (teensy_pwm_pin_info_t){1, 50, 1, 1};
  case 3:
    return (teensy_pwm_pin_info_t){1, 50, 2, 1};
  case 4:
    return (teensy_pwm_pin_info_t){1, 16, 1, 1};
  case 5:
    return (teensy_pwm_pin_info_t){1, 17, 1, 1};
  case 6:
    return (teensy_pwm_pin_info_t){1, 18, 1, 2};
  case 7:
    return (teensy_pwm_pin_info_t){1, 19, 2, 6};
  case 8:
    return (teensy_pwm_pin_info_t){1, 19, 1, 6};
  case 9:
    return (teensy_pwm_pin_info_t){1, 18, 2, 2};
  case 10:
    return (teensy_pwm_pin_info_t){2, 0, 0, 1};
  case 11:
    return (teensy_pwm_pin_info_t){2, 2, 0, 1};
  case 12:
    return (teensy_pwm_pin_info_t){2, 1, 0, 1};
  case 13:
    return (teensy_pwm_pin_info_t){2, 16, 0, 1};
  case 14:
    return (teensy_pwm_pin_info_t){2, 34, 0, 1};
  case 15:
    return (teensy_pwm_pin_info_t){2, 35, 0, 1};
  case 18:
    return (teensy_pwm_pin_info_t){2, 33, 0, 1};
  case 19:
    return (teensy_pwm_pin_info_t){2, 32, 0, 1};
  case 22:
    return (teensy_pwm_pin_info_t){1, 48, 1, 1};
  case 23:
    return (teensy_pwm_pin_info_t){1, 49, 1, 1};
  case 24:
    return (teensy_pwm_pin_info_t){1, 18, 0, 4};
  case 25:
    return (teensy_pwm_pin_info_t){1, 19, 0, 4};
  case 28:
    return (teensy_pwm_pin_info_t){1, 33, 2, 1};
  case 29:
    return (teensy_pwm_pin_info_t){1, 33, 1, 1};
  case 33:
    return (teensy_pwm_pin_info_t){1, 16, 2, 1};
  case 36:
    return (teensy_pwm_pin_info_t){1, 19, 1, 6};
  case 37:
    return (teensy_pwm_pin_info_t){1, 19, 2, 6};
  case 42:
    return (teensy_pwm_pin_info_t){1, 17, 2, 1};
  case 43:
    return (teensy_pwm_pin_info_t){1, 17, 1, 1};
  case 44:
    return (teensy_pwm_pin_info_t){1, 16, 2, 1};
  case 45:
    return (teensy_pwm_pin_info_t){1, 16, 1, 1};
  case 46:
    return (teensy_pwm_pin_info_t){1, 18, 2, 1};
  case 47:
    return (teensy_pwm_pin_info_t){1, 18, 1, 1};
  case 51:
    return (teensy_pwm_pin_info_t){1, 35, 2, 1};
  case 54:
    return (teensy_pwm_pin_info_t){1, 32, 1, 1};
  default:
    return (teensy_pwm_pin_info_t){0};
  }
}

static inline __attribute__((always_inline)) int
teensy_pwm_write_const(uint8_t pin, uint32_t value) {
  teensy_pwm_pin_info_t info = teensy_pwm_pin_const(pin);
  teensy_gpio_pin_t gpio = teensy_gpio_pin_const(pin);
  return pwm_write_info(&info, &gpio, value);
}

static inline __attribute__((always_inline)) int
teensy_pwm_set_frequency_const(uint8_t pin, float frequency_hz) {
  teensy_pwm_pin_info_t info = teensy_pwm_pin_const(pin);
  teensy_gpio_pin_t gpio = teensy_gpio_pin_const(pin);
  return pwm_set_frequency_info(&info, &gpio, frequency_hz);
}

#ifndef TEENSY_PWM_IMPLEMENTATION
extern void teensy_pwm_invalid_constant_pin(void) __attribute__((
    error("invalid Teensy PWM pin; expected a PWM-capable pin from 0 to 54")));

#define TEENSY_PWM_VALIDATE_CONSTANT_PIN(pin)                                  \
  ({                                                                           \
    if (__builtin_constant_p(pin) &&                                           \
        !((pin) == (uint8_t)(pin) && (uint8_t)(pin) < TEENSY_GPIO_PIN_COUNT && \
          ((uint8_t)(pin) < 32u                                                \
               ? ((0x33ccffffu >> (uint8_t)(pin)) & 1u) != 0u                  \
               : ((0x0048fc32u >> ((uint8_t)(pin) - 32u)) & 1u) != 0u)))       \
      teensy_pwm_invalid_constant_pin();                                       \
    (void)0;                                                                   \
  })

#define pwm_write(pin, value)                                                  \
  ({                                                                           \
    TEENSY_PWM_VALIDATE_CONSTANT_PIN(pin);                                     \
    __builtin_choose_expr(__builtin_constant_p(pin),                           \
                          teensy_pwm_write_const((uint8_t)(pin), (value)),     \
                          pwm_write((uint8_t)(pin), (value)));                 \
  })

#define pwm_set_frequency(pin, frequency_hz)                                   \
  ({                                                                           \
    TEENSY_PWM_VALIDATE_CONSTANT_PIN(pin);                                     \
    __builtin_choose_expr(                                                     \
        __builtin_constant_p(pin),                                             \
        teensy_pwm_set_frequency_const((uint8_t)(pin), (frequency_hz)),        \
        pwm_set_frequency((uint8_t)(pin), (frequency_hz)));                    \
  })
#endif

#endif
