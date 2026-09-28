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

/* ============================== PUBLIC API ============================ */

/** Enables, clocks and calibrates both ADC controllers.
 * @return 0 on success, or -1 if hardware self-calibration failed.
 */
int adc_init(void);

/** Reads a raw ADC value from an analog-capable Teensy 4.1 board pin.
 * @param pin One of the A0..A17 constants.
 * @return The conversion result in ADC counts, or -1 if the pin is not
 * analog-capable, adc_init() has not run, or the conversion timed out.
 */
static inline __attribute__((always_inline)) int adc_read(uint8_t pin);

/** Selects the ADC resolution in bits, applied to both ADC controllers.
 * @param bits 8, 10 or 12.
 * @return 0 on success, or -1 for an unsupported resolution.
 */
static inline __attribute__((always_inline)) int
adc_set_resolution(uint32_t bits);

/** Selects the hardware averaging depth in samples, applied to both ADC
 * controllers.
 * @param samples 1, 4, 8, 16 or 32.
 * @return 0 on success, or -1 for an unsupported depth.
 */
static inline __attribute__((always_inline)) int
adc_set_averaging(uint32_t samples);

typedef void (*adc_complete_handler_t)(uint16_t value, void *context);

/* ============================ INTERNAL API ============================ */
/* @internal */

/* Analog-capable pins (14-27 and 38-41) drive both validation and the
 * per-pin ADC slot in the pin map include. */

#define TEENSY_ADC_PIN_VALID(pin)                                              \
  ((pin) == (uint8_t)(pin) && (uint8_t)(pin) < TEENSY_GPIO_PIN_COUNT &&        \
   ((uint8_t)(pin) < 32u                                                       \
        ? ((0x0fffc000u >> (uint8_t)(pin)) & 1u) != 0u                         \
        : ((0x000003c0u >> ((uint8_t)(pin) - 32u)) & 1u) != 0u))

/* Set by adc_init(): conversion entry points refuse to run without it. */
extern uint8_t adc_initialized;

/* Compile-time validation: analog-capable pin values and the supported
 * resolution/averaging settings, as call-site diagnostics. */
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
#define TEENSY_ADC_VALIDATE_PIN(pin) adc_validate_pin((uint8_t)(pin))
#define TEENSY_ADC_VALIDATE_RESOLUTION(bits) adc_validate_resolution(bits)
#define TEENSY_ADC_VALIDATE_AVERAGING(samples) adc_validate_averaging((samples))
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

/* Internals backed by adc.c. */
int adc_set_resolution_impl(uint32_t bits);
int adc_set_averaging_impl(uint32_t samples);
int adc_attach_irq_impl(uint8_t instance,
                        void (*handler)(uint16_t value, void *context),
                        void *context);
int adc_trigger_impl(uint8_t instance, uint8_t channel);

/* Configures an analog pad: mux to ALT0 and clear digital keeper/bias.
 * Runs before every conversion. */
static inline __attribute__((always_inline)) void
adc_configure_pin(const gpio_pin_t *gpio) {
  *gpio->mux = 0;
  *gpio->pad &= ~(IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_ODE);
}

/* Runs one blocking conversion on an initialized ADC. */
static inline __attribute__((always_inline)) int
adc_read_channel(IMXRT_ADCS_t *adc, uint8_t channel) {
  uint32_t wait = 1000000u;

  adc->HC0 = ADC_HC_ADCH(channel);
  while ((adc->HS & ADC_HS_COCO0) == 0u && wait-- != 0u)
    __asm volatile("nop");
  if ((adc->HS & ADC_HS_COCO0) == 0u)
    return -1;
  return (int)adc->R0;
}

/* Valid (ADC, channel) pairs, taken from the pin map: ADC1 carries
 * channels 0,1,2,5..15; ADC2 only 1..4 (A12..A15).  Sampling an
 * unpopulated channel yields a floating result, so it is a
 * compile-time error for constant arguments. */
#define ADC1_CHANNELS_MASK 0xFFE7u /* 0,1,2,5,6,7,8,9,10,11,12,13,14,15 */
#define ADC2_CHANNELS_MASK 0x001Eu /* 1,2,3,4                        */
#define TEENSY_ADC_PAIR_VALID(instance, channel)                               \
  ((channel) <= 15u &&                                                         \
   (((instance) == 1u ? ADC1_CHANNELS_MASK : ADC2_CHANNELS_MASK) &             \
    (uint32_t)(1u << (channel))) != 0u)

/* Fills @p instance/@p channel for an analog-capable pin.
 * @p invalid is returned through the enclosing function when the pin
 * has no analog hardware. */
#define TEENSY_ADC_PIN_RESOLVE(instance, channel, pin, invalid)                \
  switch (pin) {                                                               \
    TEENSY_ADC_PIN_MAP(TEENSY_ADC_RESOLVE_CASE)                                \
  default:                                                                     \
    return invalid;                                                            \
  }

#define TEENSY_ADC_RESOLVE_CASE(number, _instance, _channel)                   \
  case number:                                                                 \
    (instance) = _instance;                                                    \
    (channel) = _channel;                                                      \
    break;

/* =================== PUBLIC API IMPLEMENTATIONS ======================= */

#if defined(__clang__)
static inline int adc_read(uint8_t pin) __attribute__((diagnose_if(
    !TEENSY_ADC_PIN_VALID(pin),
    "invalid Teensy ADC pin; expected an analog-capable pin", "error")));
#endif
static inline __attribute__((always_inline)) int adc_read(uint8_t pin) {
  uint8_t instance = 0;
  uint8_t channel = 0;

  TEENSY_ADC_VALIDATE_PIN(pin);
  TEENSY_ADC_PIN_RESOLVE(instance, channel, pin, -1);
  if (instance == 0 || !adc_initialized)
    return -1;
  gpio_pin_t gpio = gpio_pin_const(pin);
  adc_configure_pin(&gpio);
  return adc_read_channel(instance == 1u ? &IMXRT_ADC1 : &IMXRT_ADC2, channel);
}

#if defined(__clang__)
static inline int adc_set_resolution(uint32_t bits)
    __attribute__((diagnose_if(bits != 8u && bits != 10u && bits != 12u,
                               "ADC resolution must be 8, 10, or 12 bits",
                               "error")));
#endif
static inline
    __attribute__((always_inline)) int adc_set_resolution(uint32_t bits) {
  TEENSY_ADC_VALIDATE_RESOLUTION(bits);
  return adc_set_resolution_impl(bits);
}

#if defined(__clang__)
static inline int adc_set_averaging(uint32_t samples) __attribute__((
    diagnose_if(samples != 1u && samples != 4u && samples != 8u &&
                    samples != 16u && samples != 32u,
                "ADC averaging must be 1, 4, 8, 16, or 32", "error")));
#endif
static inline
    __attribute__((always_inline)) int adc_set_averaging(uint32_t samples) {
  TEENSY_ADC_VALIDATE_AVERAGING(samples);
  return adc_set_averaging_impl(samples);
}

/** Registers the completion handler for one ADC's adc_trigger()-started
 * conversions.  The handler runs in interrupt context and receives the
 * raw conversion result; it must be quick (no blocking).  Pass NULL to
 * detach.  One ADC must not use the interrupt and blocking paths at the
 * same time - the handler consumes the conversion result.
 * @param instance 1 (ADC1) or 2 (ADC2).
 * @param handler Handler, receives the raw value and @p context.
 * @param context Passed through to @p handler.
 * @return 0 on success, or -1 for an invalid instance.
 */
static inline __attribute__((always_inline)) int
adc_attach_irq(uint8_t instance, void (*handler)(uint16_t value, void *context),
               void *context);

/** Starts one conversion on an initialized ADC; its result is delivered
 * by the handler registered with adc_attach_irq() instead of being
 * returned to the caller.
 * @param instance 1 (ADC1) or 2 (ADC2).
 * @param channel 0..15, the ADC channel of the pin to sample (see the
 * pin map: e.g. A0 = ADC1 channel 7).
 * @return 0 on success, or -1 if the ADC is uninitialized, the
 * instance is invalid or the channel is out of range.
 */
static inline __attribute__((always_inline)) int adc_trigger(uint8_t instance,
                                                             uint8_t channel);

#if defined(__clang__)
static inline int adc_attach_irq(uint8_t instance,
                                 void (*handler)(uint16_t value, void *context),
                                 void *context)
    __attribute__((
        diagnose_if(instance != 1u && instance != 2u,
                    "invalid ADC instance; expected 1 (ADC1) or 2 (ADC2)",
                    "error"),
        diagnose_if(handler == 0,
                    "adc_attach_irq with a NULL handler detaches the "
                    "interrupt; use it deliberately or not at all",
                    "warning")));
#endif
static inline __attribute__((always_inline)) int
adc_attach_irq(uint8_t instance, void (*handler)(uint16_t value, void *context),
               void *context) {
  if (instance != 1u && instance != 2u)
    return -1;
  return adc_attach_irq_impl(instance, handler, context);
}

#if defined(__clang__)
static inline int adc_trigger(uint8_t instance, uint8_t channel) __attribute__((
    diagnose_if(instance != 1u && instance != 2u,
                "invalid ADC instance; expected 1 (ADC1) or 2 (ADC2)", "error"),
    diagnose_if(!TEENSY_ADC_PAIR_VALID(instance, channel),
                "that ADC has no such routed channel on this board",
                "error")));
#endif
static inline __attribute__((always_inline)) int adc_trigger(uint8_t instance,
                                                             uint8_t channel) {
  if ((instance != 1u && instance != 2u) ||
      !TEENSY_ADC_PAIR_VALID(instance, channel))
    return -1;
  return adc_trigger_impl(instance, channel);
}

#endif
