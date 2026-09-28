#ifndef TEENSY_GPIO_H
#define TEENSY_GPIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <teensy/gpio_pin_map.h>
#include <teensy/imxrt.h>

#define TEENSY_GPIO_PIN_COUNT 55u

typedef enum {
  GPIO_INPUT = 0,
  GPIO_OUTPUT,
  GPIO_INPUT_PULLUP,
  GPIO_INPUT_PULLDOWN,
  GPIO_OUTPUT_OPEN_DRAIN
} gpio_mode_t;

typedef enum {
  GPIO_INTERRUPT_CHANGE = 0,
  GPIO_INTERRUPT_FALLING,
  GPIO_INTERRUPT_RISING,
  GPIO_INTERRUPT_LOW,
  GPIO_INTERRUPT_HIGH
} gpio_interrupt_mode_t;

typedef void (*gpio_interrupt_handler_t)(void *context);

/* ============================== PUBLIC API ============================ */

/** Initializes GPIO interrupt handling. */
void gpio_init(void);

/** Configures a Teensy pin for the specified GPIO mode.
 * @param pin Teensy pin number, 0..54.
 * @param mode One of the gpio_mode_t values.
 * @return 0 on success, or -1 if the pin or mode is invalid.
 */

static inline __attribute__((always_inline)) int
gpio_configure(uint8_t pin, gpio_mode_t mode);

/** Drives a GPIO pin high or low.
 * @return 0 on success, or -1 if the pin is invalid.
 */

static inline __attribute__((always_inline)) int gpio_write(uint8_t pin,
                                                            bool high);

/** Stores the current logic level of a GPIO pin in @p high.
 * @return 0 on success, or -1 if the pin or output pointer is invalid.
 */

static inline __attribute__((always_inline)) int gpio_read(uint8_t pin,
                                                           bool *high);

/** Inverts the output latch of a GPIO pin.
 * @return 0 on success, or -1 if the pin is invalid.
 */

static inline __attribute__((always_inline)) int gpio_toggle(uint8_t pin);

/** Registers a callback for the configured events on a GPIO pin that has
 * already been configured with gpio_configure().
 *
 * The callback runs in interrupt context and receives @p context unchanged.
 * The pin's existing pad and direction configuration is preserved.
 *
 * @return 0 on success, or -1 if the pin, mode, or handler is invalid or
 * the pin was never configured.
 */

static inline __attribute__((always_inline)) int
gpio_attach_interrupt(uint8_t pin, gpio_interrupt_mode_t mode,
                      gpio_interrupt_handler_t handler, void *context);

/** Disables and removes the interrupt callback for a GPIO pin.
 * @return 0 on success, or -1 if the pin is invalid or was never
 * configured.
 */

static inline __attribute__((always_inline)) int
gpio_detach_interrupt(uint8_t pin);

/* ============================ INTERNAL API ============================ */
/* @internal */

typedef struct {
  volatile uint32_t *data;
  volatile uint32_t *direction;
  volatile uint32_t *input;
  volatile uint32_t *set;
  volatile uint32_t *clear;
  volatile uint32_t *toggle;
  volatile uint32_t *mux;
  volatile uint32_t *pad;
  uint32_t mask;
  uint8_t port;
  uint8_t bit;
} gpio_pin_t;

#define TEENSY_GPIO_PIN_DESCRIPTOR(port_number, pin_bit, mux_register,         \
                                   pad_register)                               \
  {&GPIO##port_number##_DR,                                                    \
   &GPIO##port_number##_GDIR,                                                  \
   &GPIO##port_number##_PSR,                                                   \
   &GPIO##port_number##_DR_SET,                                                \
   &GPIO##port_number##_DR_CLEAR,                                              \
   &GPIO##port_number##_DR_TOGGLE,                                             \
   &(mux_register),                                                            \
   &(pad_register),                                                            \
   (uint32_t)1u << (pin_bit),                                                  \
   (uint8_t)((port_number) - 6u),                                              \
   (uint8_t)(pin_bit)}

/* Compile-time pin validation: call-site diagnose_if for language
 * servers, unreachable folded error branch for GCC. */
#if defined(__clang__)
static inline void gpio_validate(uint8_t pin) __attribute__((diagnose_if(
    pin >= TEENSY_GPIO_PIN_COUNT,
    "invalid Teensy GPIO pin; expected a value from 0 to 54", "error")));
static inline void gpio_validate(uint8_t pin) { (void)pin; }
#else
extern void gpio_invalid_constant_pin(void) __attribute__((
    error("invalid Teensy GPIO pin; expected a value from 0 to 54")));
#endif

#if defined(__clang__)
#define TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin) gpio_validate((uint8_t)(pin))
#else
#define TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin)                                 \
  ({                                                                           \
    if (__builtin_constant_p(pin) &&                                           \
        !((pin) == (uint8_t)(pin) && (uint8_t)(pin) < TEENSY_GPIO_PIN_COUNT))  \
      gpio_invalid_constant_pin();                                             \
    (void)0;                                                                   \
  })
#endif

/* Fills the OUT var @p d with the pin's descriptor.  @p d must be a
 * local `gpio_pin_t` declared before this expansion; @p on_invalid is
 * the return expression used when the pin is out of range. */
#define TEENSY_GPIO_PIN_RESOLVE(d, pin, on_invalid)                            \
  switch (pin) {                                                               \
    TEENSY_GPIO_PIN_MAP(TEENSY_GPIO_PIN_FILL_CASE)                             \
  default:                                                                     \
    return on_invalid;                                                         \
  }

#define TEENSY_GPIO_PIN_FILL_CASE(number, port_number, pin_bit, mux_register,  \
                                  pad_register)                                \
  case number:                                                                 \
    (d) = (gpio_pin_t){&GPIO##port_number##_DR,                                \
                       &GPIO##port_number##_GDIR,                              \
                       &GPIO##port_number##_PSR,                               \
                       &GPIO##port_number##_DR_SET,                            \
                       &GPIO##port_number##_DR_CLEAR,                          \
                       &GPIO##port_number##_DR_TOGGLE,                         \
                       &(mux_register),                                        \
                       &(pad_register),                                        \
                       (uint32_t)1u << (pin_bit),                              \
                       (uint8_t)((port_number) - 6u),                          \
                       (uint8_t)(pin_bit)};                                    \
    break;

/* Compile-time descriptor builder used by the adc/pwm modules and the
 * public inlines above. */
static inline __attribute__((always_inline)) gpio_pin_t
gpio_pin_const(uint8_t pin) {
  gpio_pin_t d = {0};
  TEENSY_GPIO_PIN_RESOLVE(d, pin, ((gpio_pin_t){0}));
  return d;
}

const gpio_pin_t *gpio_pin_runtime(uint8_t pin);
int gpio_configure_pin(const gpio_pin_t *pin, gpio_mode_t mode);
int gpio_attach_interrupt_pin(const gpio_pin_t *pin, gpio_interrupt_mode_t mode,
                              gpio_interrupt_handler_t handler, void *context);
int gpio_detach_interrupt_pin(const gpio_pin_t *pin);
static inline __attribute__((always_inline)) uint32_t
gpio_pad_for_mode(gpio_mode_t mode) {
  switch (mode) {
  case GPIO_INPUT_PULLUP:
    return IOMUXC_PAD_DSE(7) | IOMUXC_PAD_PKE | IOMUXC_PAD_PUE |
           IOMUXC_PAD_PUS(3) | IOMUXC_PAD_HYS;
  case GPIO_INPUT_PULLDOWN:
    return IOMUXC_PAD_DSE(7) | IOMUXC_PAD_PKE | IOMUXC_PAD_PUE |
           IOMUXC_PAD_PUS(0) | IOMUXC_PAD_HYS;
  case GPIO_OUTPUT_OPEN_DRAIN:
    return IOMUXC_PAD_DSE(7) | IOMUXC_PAD_ODE;
  case GPIO_INPUT:
  case GPIO_OUTPUT:
    return IOMUXC_PAD_DSE(7);
  default:
    return 0;
  }
}

/* =================== PUBLIC API IMPLEMENTATIONS ======================= */

static inline __attribute__((always_inline)) int
gpio_configure(uint8_t pin, gpio_mode_t mode) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);
  gpio_pin_t d;
  TEENSY_GPIO_PIN_RESOLVE(d, pin, -1);
  return gpio_configure_pin(&d, mode);
}

static inline __attribute__((always_inline)) int gpio_write(uint8_t pin,
                                                            bool high) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);
  gpio_pin_t d;
  TEENSY_GPIO_PIN_RESOLVE(d, pin, -1);
  if (high)
    *d.set = d.mask;
  else
    *d.clear = d.mask;
  return 0;
}

static inline __attribute__((always_inline)) int gpio_read(uint8_t pin,
                                                           bool *high) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);
  gpio_pin_t d;
  TEENSY_GPIO_PIN_RESOLVE(d, pin, -1);
  if (high == NULL)
    return -1;
  *high = (*d.input & d.mask) != 0;
  return 0;
}

static inline __attribute__((always_inline)) int gpio_toggle(uint8_t pin) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);
  gpio_pin_t d;
  TEENSY_GPIO_PIN_RESOLVE(d, pin, -1);
  *d.toggle = d.mask;
  return 0;
}

static inline __attribute__((always_inline)) int
gpio_attach_interrupt(uint8_t pin, gpio_interrupt_mode_t mode,
                      gpio_interrupt_handler_t handler, void *context) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);
  gpio_pin_t d;
  TEENSY_GPIO_PIN_RESOLVE(d, pin, -1);
  return gpio_attach_interrupt_pin(&d, mode, handler, context);
}

static inline __attribute__((always_inline)) int
gpio_detach_interrupt(uint8_t pin) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);
  gpio_pin_t d;
  TEENSY_GPIO_PIN_RESOLVE(d, pin, -1);
  return gpio_detach_interrupt_pin(&d);
}

#endif
