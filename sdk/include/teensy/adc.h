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

/* Internals backed by adc.c. */
int adc_set_resolution_impl(uint32_t bits);
int adc_set_averaging_impl(uint32_t samples);

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

static inline __attribute__((always_inline)) int adc_read(uint8_t pin) {
  uint8_t instance = 0;
  uint8_t channel = 0;

  TEENSY_ADC_PIN_RESOLVE(instance, channel, pin, -1);
  if (instance == 0 || !adc_initialized)
    return -1;
  gpio_pin_t gpio = gpio_pin_const(pin);
  adc_configure_pin(&gpio);
  return adc_read_channel(instance == 1u ? &IMXRT_ADC1 : &IMXRT_ADC2, channel);
}

static inline __attribute__((always_inline)) int
adc_set_resolution(uint32_t bits) {
  return adc_set_resolution_impl(bits);
}

static inline __attribute__((always_inline)) int
adc_set_averaging(uint32_t samples) {
  return adc_set_averaging_impl(samples);
}

#endif
