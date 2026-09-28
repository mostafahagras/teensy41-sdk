#ifndef TEENSY_PWM_H
#define TEENSY_PWM_H

#include <stdint.h>

#include <teensy/clock.h>
#include <teensy/gpio.h>
#include <teensy/imxrt.h>
#include <teensy/pwm_pin_map.h>

/* ============================== PUBLIC API ============================ */

/** Initializes all PWM controller clocks, submodules and channels. */
void pwm_init(void);

/** Sets a PWM pin's duty value using the current resolution.
 * @param pin A PWM-capable Teensy 4.1 pin.
 * @param value Duty value; values above the current resolution's
 * maximum are clamped.
 * @return 0 on success, or -1 if the pin does not support PWM.
 */
static inline __attribute__((always_inline)) int pwm_write(uint8_t pin,
                                                           uint32_t value);

/** Sets the PWM frequency for the timer associated with a pin.
 * @param pin A PWM-capable Teensy 4.1 pin.
 * @param frequency_hz Positive frequency in hertz.
 * @return 0 on success, or -1 if the pin or the frequency is invalid.
 *
 * Other pins sharing the same hardware timer may also be affected.
 */
static inline __attribute__((always_inline)) int
pwm_set_frequency(uint8_t pin, float frequency_hz);

/** Sets the global duty-cycle resolution in bits.
 *
 * The requested value is clamped to the supported range of 1..16 bits.
 *
 * @return The previous resolution in bits.
 */
static inline __attribute__((always_inline)) uint32_t
pwm_set_resolution(uint32_t bits);

/* ============================ INTERNAL API ============================ */
/* @internal */

typedef struct {
  uint8_t type;
  uint8_t module;
  uint8_t channel;
  uint8_t muxval;
} pwm_pin_info_t;

/* Bits set for PWM-capable pins; the low mask covers pins 0..31, the
 * high mask pins 32..54 (shifted by 32). */
#define PWM_CAPABLE_SET_L 0x33ccffffu
#define PWM_CAPABLE_SET_H 0x0048fc32u

#define TEENSY_PWM_PIN_VALID(pin)                                              \
  ((pin) == (uint8_t)(pin) && (uint8_t)(pin) < TEENSY_GPIO_PIN_COUNT &&        \
   ((uint8_t)(pin) < 32u                                                       \
        ? ((PWM_CAPABLE_SET_L >> (uint8_t)(pin)) & 1u) != 0u                   \
        : ((PWM_CAPABLE_SET_H >> ((uint8_t)(pin) - 32u)) & 1u) != 0u))

/* Compile-time validation. */
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

/* Register-block math stays in pwm.c (it reads live hardware state --
 * VAL1/LOAD -- plus the global duty resolution); the pin configuration
 * happens inline here so a constant pin costs only scalar stores +
 * one call with folded arguments. */
int pwm_write_flex(IMXRT_FLEXPWM_t *p, uint8_t submodule, uint8_t channel,
                   uint16_t value);
int pwm_write_quad(IMXRT_TMR_t *p, uint8_t submodule, uint16_t value);
int pwm_frequency_flex(IMXRT_FLEXPWM_t *p, uint8_t submodule, uint8_t channel,
                       float frequency_hz);
int pwm_frequency_quad(IMXRT_TMR_t *p, uint8_t submodule, float frequency_hz);

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

#define TEENSY_PWM_WRITE_CASE(number, type, module, channel, muxval)           \
  case number:                                                                 \
    if ((type) == 1) {                                                         \
      pwm_pin_output(&gpio, muxval);                                           \
      return pwm_write_flex(TEENSY_PWM_FLEX_POINTER_##module, (module) & 3u,   \
                            channel, (uint16_t)value);                         \
    }                                                                          \
    if ((type) == 2) {                                                         \
      pwm_pin_output(&gpio, muxval);                                           \
      return pwm_write_quad(TEENSY_PWM_QUAD_POINTER_##module, (module) & 3u,   \
                            (uint16_t)value);                                  \
    }                                                                          \
    return -1;

#define TEENSY_PWM_FREQUENCY_CASE(number, type, module, channel, muxval)       \
  case number:                                                                 \
    if ((type) == 1) {                                                         \
      pwm_pin_output(&gpio, muxval);                                           \
      return pwm_frequency_flex(TEENSY_PWM_FLEX_POINTER_##module,              \
                                (module) & 3u, channel, frequency_hz);         \
    }                                                                          \
    if ((type) == 2) {                                                         \
      pwm_pin_output(&gpio, muxval);                                           \
      return pwm_frequency_quad(TEENSY_PWM_QUAD_POINTER_##module,              \
                                (module) & 3u, frequency_hz);                  \
    }                                                                          \
    return -1;

uint32_t pwm_set_resolution_impl(uint32_t bits);
int pwm_write_impl(uint8_t pin, uint32_t value);
int pwm_set_frequency_impl(uint8_t pin, float frequency_hz);

/* Routes a pin to its PWM function: direction=output, pad electricals
 * = plain output drive, mux = the pin's PWM alternate function. */
static inline __attribute__((always_inline)) void
pwm_pin_output(gpio_pin_t *gpio, uint8_t muxval) {
  *gpio->direction |= gpio->mask;
  *gpio->pad = gpio_pad_for_mode(GPIO_OUTPUT);
  *gpio->mux = muxval;
}

/* =================== PUBLIC API IMPLEMENTATIONS ======================= */

#if defined(__clang__)
static inline int pwm_write(uint8_t pin, uint32_t value)
    __attribute__((diagnose_if(
        !TEENSY_PWM_PIN_VALID(pin),
        "invalid Teensy PWM pin; expected a PWM-capable pin from 0 to 54",
        "error")));
#endif
static inline
    __attribute__((always_inline)) int pwm_write(uint8_t pin, uint32_t value) {
  gpio_pin_t gpio;

  TEENSY_PWM_VALIDATE_CONSTANT_PIN(pin);
  gpio = gpio_pin_const(pin);

  switch (pin) {
    TEENSY_PWM_PIN_MAP(TEENSY_PWM_WRITE_CASE)
  default:
    return -1;
  }
}

#if defined(__clang__)
static inline int pwm_set_frequency(uint8_t pin, float frequency_hz)
    __attribute__((
        diagnose_if(
            !TEENSY_PWM_PIN_VALID(pin),
            "invalid Teensy PWM pin; expected a PWM-capable pin from 0 to 54",
            "error"),
        diagnose_if(frequency_hz <= 0.0f,
                    "PWM frequency must be greater than zero", "error")));
#endif
static inline
    __attribute__((always_inline)) int pwm_set_frequency(uint8_t pin,
                                                         float frequency_hz) {
  gpio_pin_t gpio;

  TEENSY_PWM_VALIDATE_CONSTANT_PIN(pin);
  TEENSY_PWM_VALIDATE_FREQUENCY(frequency_hz);
  gpio = gpio_pin_const(pin);

  switch (pin) {
    TEENSY_PWM_PIN_MAP(TEENSY_PWM_FREQUENCY_CASE)
  default:
    return -1;
  }
}

#if defined(__clang__)
static inline uint32_t pwm_set_resolution(uint32_t bits)
    __attribute__((diagnose_if(bits < 1u || bits > 16u,
                               "PWM resolution is clamped to the range 1..16",
                               "warning")));
#endif
static inline
    __attribute__((always_inline)) uint32_t pwm_set_resolution(uint32_t bits) {
  TEENSY_PWM_VALIDATE_RESOLUTION(bits);
  return pwm_set_resolution_impl(bits);
}

#endif
