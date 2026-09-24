#ifndef TEENSY_PWM_H
#define TEENSY_PWM_H

#include <stdint.h>

#include <teensy/gpio.h>
#include <teensy/imxrt.h>
#include <teensy/pwm_pin_map.h>

typedef struct {
  uint8_t type;
  uint8_t module;
  uint8_t channel;
  uint8_t muxval;
} pwm_pin_info_t;

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

int pwm_write_flex(IMXRT_FLEXPWM_t *p, uint8_t submodule, uint8_t channel,
                   uint8_t muxval, const gpio_pin_t *gpio, uint32_t value);
int pwm_write_quad(IMXRT_TMR_t *p, uint8_t submodule, uint8_t muxval,
                   const gpio_pin_t *gpio, uint32_t value);
int pwm_frequency_flex(IMXRT_FLEXPWM_t *p, uint8_t submodule, uint8_t channel,
                       uint8_t muxval, const gpio_pin_t *gpio,
                       float frequency_hz);
int pwm_frequency_quad(IMXRT_TMR_t *p, uint8_t submodule, uint8_t muxval,
                       const gpio_pin_t *gpio, float frequency_hz);

#define TEENSY_PWM_FLEX_POINTER_0 &IMXRT_FLEXPWM1
#define TEENSY_PWM_FLEX_POINTER_1 &IMXRT_FLEXPWM1
#define TEENSY_PWM_FLEX_POINTER_2 &IMXRT_FLEXPWM1
#define TEENSY_PWM_FLEX_POINTER_16 &IMXRT_FLEXPWM2
#define TEENSY_PWM_FLEX_POINTER_17 &IMXRT_FLEXPWM2
#define TEENSY_PWM_FLEX_POINTER_18 &IMXRT_FLEXPWM2
#define TEENSY_PWM_FLEX_POINTER_19 &IMXRT_FLEXPWM2
#define TEENSY_PWM_FLEX_POINTER_32 &IMXRT_FLEXPWM3
#define TEENSY_PWM_FLEX_POINTER_33 &IMXRT_FLEXPWM3
#define TEENSY_PWM_FLEX_POINTER_34 &IMXRT_FLEXPWM3
#define TEENSY_PWM_FLEX_POINTER_35 &IMXRT_FLEXPWM3
#define TEENSY_PWM_FLEX_POINTER_48 &IMXRT_FLEXPWM4
#define TEENSY_PWM_FLEX_POINTER_49 &IMXRT_FLEXPWM4
#define TEENSY_PWM_FLEX_POINTER_50 &IMXRT_FLEXPWM4

#define TEENSY_PWM_QUAD_POINTER_0 &IMXRT_TMR1
#define TEENSY_PWM_QUAD_POINTER_1 &IMXRT_TMR1
#define TEENSY_PWM_QUAD_POINTER_2 &IMXRT_TMR1
#define TEENSY_PWM_QUAD_POINTER_16 &IMXRT_TMR2
#define TEENSY_PWM_QUAD_POINTER_17 &IMXRT_TMR2
#define TEENSY_PWM_QUAD_POINTER_18 &IMXRT_TMR2
#define TEENSY_PWM_QUAD_POINTER_19 &IMXRT_TMR2
#define TEENSY_PWM_QUAD_POINTER_32 &IMXRT_TMR3
#define TEENSY_PWM_QUAD_POINTER_33 &IMXRT_TMR3
#define TEENSY_PWM_QUAD_POINTER_34 &IMXRT_TMR3
#define TEENSY_PWM_QUAD_POINTER_35 &IMXRT_TMR3
#define TEENSY_PWM_QUAD_POINTER_48 &IMXRT_TMR4
#define TEENSY_PWM_QUAD_POINTER_49 &IMXRT_TMR4
#define TEENSY_PWM_QUAD_POINTER_50 &IMXRT_TMR4

static inline
    __attribute__((always_inline)) int pwm_write_const(uint8_t pin,
                                                       uint32_t value) {
  gpio_pin_t gpio = gpio_pin_const(pin);

#define TEENSY_PWM_WRITE_CASE(number, type, module, channel, muxval)           \
  case number:                                                                 \
    if (type == 1)                                                             \
      return pwm_write_flex(TEENSY_PWM_FLEX_POINTER_##module, (module) & 3u,   \
                            channel, muxval, &gpio, value);                    \
    if (type == 2)                                                             \
      return pwm_write_quad(TEENSY_PWM_QUAD_POINTER_##module, (module) & 3u,   \
                            muxval, &gpio, value);                             \
    return -1;

  switch (pin) {
    TEENSY_PWM_PIN_MAP(TEENSY_PWM_WRITE_CASE)
  default:
    return -1;
  }
#undef TEENSY_PWM_WRITE_CASE
}

static inline __attribute__((always_inline)) int
pwm_set_frequency_const(uint8_t pin, float frequency_hz) {
  gpio_pin_t gpio = gpio_pin_const(pin);

#define TEENSY_PWM_FREQUENCY_CASE(number, type, module, channel, muxval)       \
  case number:                                                                 \
    if (type == 1)                                                             \
      return pwm_frequency_flex(TEENSY_PWM_FLEX_POINTER_##module,              \
                                (module) & 3u, channel, muxval, &gpio,         \
                                frequency_hz);                                 \
    if (type == 2)                                                             \
      return pwm_frequency_quad(TEENSY_PWM_QUAD_POINTER_##module,              \
                                (module) & 3u, muxval, &gpio, frequency_hz);   \
    return -1;

  switch (pin) {
    TEENSY_PWM_PIN_MAP(TEENSY_PWM_FREQUENCY_CASE)
  default:
    return -1;
  }
#undef TEENSY_PWM_FREQUENCY_CASE
}

#ifndef TEENSY_PWM_IMPLEMENTATION
#define TEENSY_PWM_PIN_VALID(pin)                                              \
  ((pin) == (uint8_t)(pin) && (uint8_t)(pin) < TEENSY_GPIO_PIN_COUNT &&        \
   ((uint8_t)(pin) < 32u                                                       \
        ? ((0x33ccffffu >> (uint8_t)(pin)) & 1u) != 0u                         \
        : ((0x0048fc32u >> ((uint8_t)(pin) - 32u)) & 1u) != 0u))

#if defined(__clang__)
static inline void pwm_validate(uint8_t pin) __attribute__((diagnose_if(
    !TEENSY_PWM_PIN_VALID(pin),
    "invalid Teensy PWM pin; expected a PWM-capable pin from 0 to 54",
    "error")));
static inline void pwm_validate(uint8_t pin) { (void)pin; }
static inline void pwm_validate_frequency(float frequency_hz)
    __attribute__((diagnose_if(frequency_hz <= 0.0f,
                               "PWM frequency must be greater than zero",
                               "error")));
static inline void pwm_validate_frequency(float frequency_hz) {
  (void)frequency_hz;
}
static inline void pwm_validate_resolution(uint32_t bits)
    __attribute__((diagnose_if(bits < 1u || bits > 16u,
                               "PWM resolution is clamped to the range 1..16",
                               "warning")));
static inline void pwm_validate_resolution(uint32_t bits) { (void)bits; }
#else
extern void pwm_invalid_constant_pin(void) __attribute__((
    error("invalid Teensy PWM pin; expected a PWM-capable pin from 0 to 54")));
extern void pwm_invalid_frequency(void)
    __attribute__((error("PWM frequency must be greater than zero")));
static inline void pwm_invalid_resolution(uint32_t bits)
    __attribute__((warning("PWM resolution is clamped to the range 1..16")));
static inline void pwm_invalid_resolution(uint32_t bits) { (void)bits; }
#endif

#if defined(__clang__)
#define TEENSY_PWM_VALIDATE_CONSTANT_PIN(pin) pwm_validate((uint8_t)(pin))
#define TEENSY_PWM_VALIDATE_FREQUENCY(frequency_hz)                            \
  pwm_validate_frequency((frequency_hz))
#define TEENSY_PWM_VALIDATE_RESOLUTION(bits) pwm_validate_resolution((bits))
#else
#define TEENSY_PWM_VALIDATE_CONSTANT_PIN(pin)                                  \
  ({                                                                           \
    if (__builtin_constant_p(pin) && !TEENSY_PWM_PIN_VALID(pin))               \
      pwm_invalid_constant_pin();                                              \
    (void)0;                                                                   \
  })
#define TEENSY_PWM_VALIDATE_FREQUENCY(frequency_hz)                            \
  ({                                                                           \
    if (__builtin_constant_p(frequency_hz) && (frequency_hz) <= 0.0f)          \
      pwm_invalid_frequency();                                                 \
    (void)0;                                                                   \
  })
#define TEENSY_PWM_VALIDATE_RESOLUTION(bits)                                   \
  ({                                                                           \
    if (__builtin_constant_p(bits) && ((bits) < 1u || (bits) > 16u))           \
      pwm_invalid_resolution((bits));                                          \
    (void)0;                                                                   \
  })
#endif

#define pwm_write(pin, value)                                                  \
  ({                                                                           \
    TEENSY_PWM_VALIDATE_CONSTANT_PIN(pin);                                     \
    __builtin_choose_expr(__builtin_constant_p(pin),                           \
                          pwm_write_const((uint8_t)(pin), (value)),            \
                          pwm_write((uint8_t)(pin), (value)));                 \
  })

#define pwm_set_frequency(pin, frequency_hz)                                   \
  ({                                                                           \
    TEENSY_PWM_VALIDATE_CONSTANT_PIN(pin);                                     \
    TEENSY_PWM_VALIDATE_FREQUENCY(frequency_hz);                               \
    __builtin_choose_expr(                                                     \
        __builtin_constant_p(pin),                                             \
        pwm_set_frequency_const((uint8_t)(pin), (frequency_hz)),               \
        pwm_set_frequency((uint8_t)(pin), (frequency_hz)));                    \
  })

#define pwm_set_resolution(bits)                                               \
  ({                                                                           \
    TEENSY_PWM_VALIDATE_RESOLUTION(bits);                                      \
    pwm_set_resolution((bits));                                                \
  })
#endif

#endif
