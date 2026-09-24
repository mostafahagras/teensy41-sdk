#ifndef TEENSY_ADC_H
#define TEENSY_ADC_H

#include <stdint.h>

#include <teensy/adc_pin_map.h>
#include <teensy/gpio.h>
#include <teensy/imxrt.h>

#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19
#define A6 20
#define A7 21
#define A8 22
#define A9 23
#define A10 24
#define A11 25
#define A12 26
#define A13 27
#define A14 38
#define A15 39
#define A16 40
#define A17 41

/** Enables and calibrates both ADC controllers. */
int adc_init(void);

/** Reads a raw ADC value from a Teensy 4.1 analog-capable board pin. */
int adc_read(uint8_t pin);

/** Selects 8-, 10-, or 12-bit conversions. */
int adc_set_resolution(uint32_t bits);

/** Selects hardware averaging of 1, 4, 8, 16, or 32 samples. */
int adc_set_averaging(uint32_t samples);

/* Internal fixed/runtime conversion entry point. */
int adc_read_channel(IMXRT_ADCS_t *adc, uint8_t channel,
                     const gpio_pin_t *gpio);

#define TEENSY_ADC_PIN_VALID(pin)                                              \
  ((pin) == (uint8_t)(pin) && (uint8_t)(pin) < TEENSY_GPIO_PIN_COUNT &&        \
   ((uint8_t)(pin) < 32u                                                       \
        ? ((0x0fffc000u >> (uint8_t)(pin)) & 1u) != 0u                         \
        : ((0x000003c0u >> ((uint8_t)(pin) - 32u)) & 1u) != 0u))

static inline __attribute__((always_inline)) int adc_read_const(uint8_t pin) {
  gpio_pin_t gpio = gpio_pin_const(pin);

#define TEENSY_ADC_READ_CASE(number, instance, channel)                        \
  case number:                                                                 \
    return adc_read_channel(instance == 1 ? &IMXRT_ADC1 : &IMXRT_ADC2,         \
                            channel, &gpio);

  switch (pin) {
    TEENSY_ADC_PIN_MAP(TEENSY_ADC_READ_CASE)
  default:
    return -1;
  }
#undef TEENSY_ADC_READ_CASE
}

#ifndef TEENSY_ADC_IMPLEMENTATION
#if defined(__clang__)
static inline void adc_validate_pin(uint32_t pin) __attribute__((diagnose_if(
    !TEENSY_ADC_PIN_VALID(pin),
    "invalid Teensy ADC pin; expected an analog-capable pin", "error")));
static inline void adc_validate_pin(uint32_t pin) { (void)pin; }
static inline void adc_validate_resolution(uint32_t bits)
    __attribute__((diagnose_if(bits != 8u && bits != 10u && bits != 12u,
                               "ADC resolution must be 8, 10, or 12 bits",
                               "error")));
static inline void adc_validate_resolution(uint32_t bits) { (void)bits; }
static inline void adc_validate_averaging(uint32_t samples) __attribute__((
    diagnose_if(samples != 1u && samples != 4u && samples != 8u &&
                    samples != 16u && samples != 32u,
                "ADC averaging must be 1, 4, 8, 16, or 32", "error")));
static inline void adc_validate_averaging(uint32_t samples) { (void)samples; }
#else
extern void adc_invalid_pin(void) __attribute__((
    error("invalid Teensy ADC pin; expected an analog-capable pin")));
extern void adc_invalid_resolution(void)
    __attribute__((error("ADC resolution must be 8, 10, or 12 bits")));
extern void adc_invalid_averaging(void)
    __attribute__((error("ADC averaging must be 1, 4, 8, 16, or 32")));
#endif

#if defined(__clang__)
#define TEENSY_ADC_VALIDATE_PIN(pin) adc_validate_pin((pin))
#define TEENSY_ADC_VALIDATE_RESOLUTION(bits) adc_validate_resolution(bits)
#define TEENSY_ADC_VALIDATE_AVERAGING(samples) adc_validate_averaging(samples)
#else
#define TEENSY_ADC_VALIDATE_PIN(pin)                                           \
  ({                                                                           \
    if (__builtin_constant_p(pin) && !TEENSY_ADC_PIN_VALID(pin))               \
      adc_invalid_pin();                                                       \
    (void)0;                                                                   \
  })
#define TEENSY_ADC_VALIDATE_RESOLUTION(bits)                                   \
  ({                                                                           \
    if (__builtin_constant_p(bits) &&                                          \
        ((bits) != 8u && (bits) != 10u && (bits) != 12u))                      \
      adc_invalid_resolution();                                                \
    (void)0;                                                                   \
  })
#define TEENSY_ADC_VALIDATE_AVERAGING(samples)                                 \
  ({                                                                           \
    if (__builtin_constant_p(samples) &&                                       \
        ((samples) != 1u && (samples) != 4u && (samples) != 8u &&              \
         (samples) != 16u && (samples) != 32u))                                \
      adc_invalid_averaging();                                                 \
    (void)0;                                                                   \
  })
#endif

#define adc_read(pin)                                                          \
  ({                                                                           \
    TEENSY_ADC_VALIDATE_PIN(pin);                                              \
    __builtin_choose_expr(__builtin_constant_p(pin),                           \
                          adc_read_const((uint8_t)(pin)),                      \
                          adc_read((uint8_t)(pin)));                           \
  })

#define adc_set_resolution(bits)                                               \
  ({                                                                           \
    TEENSY_ADC_VALIDATE_RESOLUTION(bits);                                      \
    adc_set_resolution((bits));                                                \
  })

#define adc_set_averaging(samples)                                             \
  ({                                                                           \
    TEENSY_ADC_VALIDATE_AVERAGING(samples);                                    \
    adc_set_averaging((samples));                                              \
  })
#endif

#endif
