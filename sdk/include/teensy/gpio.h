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

/* Values deliberately start at 1 so the range checks in the bank
 * functions cannot be optimized into always-true comparisons. */
typedef enum {
  GPIO_PORT_6 = 1, /* AD_B0 / AD_B1 pins */
  GPIO_PORT_7,     /* B0 / B1 pins          */
  GPIO_PORT_8,     /* SD_B0 + odd EMC pins  */
  GPIO_PORT_9      /* EMC pins              */
} gpio_port_t;

typedef void (*gpio_interrupt_handler_t)(void *context);

/* The 55 Teensy header pins live in four 32-bit GPIO banks.  A bank is
 * the unit of the *_mask() operations below: any subset of a bank's pins
 * can be driven with a single atomic register write.
 *
 * Bank GPIO_PORT_6 (pins 0, 1, 14-27, 38-41):
 *   bit 2  pin 1   bit12 pin 24  bit16 pin 19  bit22 pin 17  bit24 pin 22
 *   bit 3  pin 0   bit13 pin 25  bit17 pin 18  bit23 pin 16  bit25 pin 23
 *   bit16 pin 19   bit18 pin 14  bit20 pin 40  bit26 pin 20  bit28 pin 38
 *   bit19 pin 15   bit21 pin 41  bit27 pin 21  bit29 pin 39  bit30 pin 26
 *                            ...bits 30/31: pins 26/27
 *
 * Bank GPIO_PORT_7 (pins 6-13, 32, 34-37):
 *   bit 0 pin 10   bit 1 pin 12   bit 2 pin 11   bit 3 pin 13 (LED)
 *   bit10 pin  6   bit11 pin  9   bit12 pin 32
 *   bit16 pin  8   bit17 pin  7   bit18 pin 36   bit19 pin 37
 *   bit28 pin 35   bit29 pin 34
 *
 * Bank GPIO_PORT_8 (pins 28, 30, 31, 42-47):
 *   bit12 pin 45   bit13 pin 44   bit14 pin 43   bit15 pin 42
 *   bit16 pin 47   bit17 pin 46   bit18 pin 28   bit22 pin 31 bit23 pin 30
 *
 * Bank GPIO_PORT_9 (pins 2-5, 29, 33, 48-54):
 *   bit 4 pin 2    bit 5 pin 3    bit 6 pin 4    bit 7 pin 33
 *   bit 8 pin 5    bit22 pin 51   bit24 pin 48   bit25 pin 53
 *   bit26 pin 52   bit27 pin 49   bit28 pin 50   bit29 pin 54 bit31 pin 29
 */

/* ============================== PUBLIC API ============================ */

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

/** Returns the current logic level of a GPIO pin.
 * @param pin Teensy pin number.
 * @return The pin state (true = high, false = low).  An invalid pin
 * reads as false; with a constant pin out of range the build fails at
 * compile time instead.
 */
static inline __attribute__((always_inline)) bool gpio_read(uint8_t pin);

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

/* Compile-time GPIO bank validation. */
#if defined(__clang__)
static inline void gpio_validate_port(gpio_port_t port) __attribute__((
    diagnose_if(port < GPIO_PORT_6 || port > GPIO_PORT_9,
                "invalid GPIO bank; expected GPIO_PORT_6 through GPIO_PORT_9",
                "error")));
static inline void gpio_validate_port(gpio_port_t port) { (void)port; }
#else
extern void gpio_invalid_constant_port(void) __attribute__((
    error("invalid GPIO bank; expected GPIO_PORT_6 through GPIO_PORT_9")));
#endif

#if defined(__clang__)
#define TEENSY_GPIO_VALIDATE_CONSTANT_PORT(port)                               \
  gpio_validate_port((gpio_port_t)(port))
#else
#define TEENSY_GPIO_VALIDATE_CONSTANT_PORT(port)                               \
  ({                                                                           \
    if (__builtin_constant_p(port) &&                                          \
        !((port) >= GPIO_PORT_6 && (port) <= GPIO_PORT_9))                     \
      gpio_invalid_constant_port();                                            \
    (void)0;                                                                   \
  })
#endif

const gpio_pin_t *gpio_pin_runtime(uint8_t pin);

int gpio_attach_interrupt_pin(const gpio_pin_t *pin, gpio_interrupt_mode_t mode,
                              gpio_interrupt_handler_t handler, void *context);
int gpio_detach_interrupt_pin(const gpio_pin_t *pin);
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

static inline __attribute__((always_inline)) int
gpio_configure_pin(const gpio_pin_t *pin, gpio_mode_t mode) {
  uint32_t pad;

  if (pin == NULL || mode > GPIO_OUTPUT_OPEN_DRAIN)
    return -1;
  pad = gpio_pad_for_mode(mode);
  if (mode == GPIO_OUTPUT || mode == GPIO_OUTPUT_OPEN_DRAIN) {
    *pin->direction |= pin->mask;
  } else {
    *pin->direction &= ~pin->mask;
  }
  *pin->pad = pad;
  *pin->mux = 5u | 0x10u;
  return 0;
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

static inline __attribute__((always_inline)) bool gpio_read(uint8_t pin) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);
  gpio_pin_t d;
  TEENSY_GPIO_PIN_RESOLVE(d, pin, false);
  return (*d.input & d.mask) != 0;
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

/** \'Drives a subset of one GPIO bank high in a single atomic register
 * #write; every other bit of the mask is left untouched.
 * @param port One of the GPIO_PORT_6..GPIO_PORT_9 constants.
 * @param mask Bit mask selecting the pins to drive high.
 */
#if defined(__clang__)
static inline int gpio_set_mask(gpio_port_t port, uint32_t mask)
    __attribute__((diagnose_if(port < GPIO_PORT_6 || port > GPIO_PORT_9,
                               "invalid GPIO port; expected GPIO_PORT_6 "
                               "through GPIO_PORT_9",
                               "error")));
#endif
static inline __attribute__((always_inline)) int gpio_set_mask(gpio_port_t port,
                                                               uint32_t mask) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PORT(port);
  switch (port) {
  case GPIO_PORT_6:
    GPIO6_DR_SET = mask;
    break;
  case GPIO_PORT_7:
    GPIO7_DR_SET = mask;
    break;
  case GPIO_PORT_8:
    GPIO8_DR_SET = mask;
    break;
  case GPIO_PORT_9:
    GPIO9_DR_SET = mask;
    break;
  default:
    return -1;
  }
  return 0;
}

/** Drives a subset of one GPIO bank low atomically; other bits unchanged.
 * @param port One of the GPIO_PORT_6..GPIO_PORT_9 constants.
 * @param mask Bit mask selecting the pins to drive low.
 */
static inline __attribute__((always_inline)) int
gpio_clear_mask(gpio_port_t port, uint32_t mask) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PORT(port);
  switch (port) {
  case GPIO_PORT_6:
    GPIO6_DR_CLEAR = mask;
    break;
  case GPIO_PORT_7:
    GPIO7_DR_CLEAR = mask;
    break;
  case GPIO_PORT_8:
    GPIO8_DR_CLEAR = mask;
    break;
  case GPIO_PORT_9:
    GPIO9_DR_CLEAR = mask;
    break;
  default:
    return -1;
  }
  return 0;
}

/** Inverts a subset of one GPIO bank atomically; other bits unchanged.
 * @param port One of the GPIO_PORT_6..GPIO_PORT_9 constants.
 * @param mask Bit mask selecting the pins to invert.
 */
static inline __attribute__((always_inline)) int
gpio_toggle_mask(gpio_port_t port, uint32_t mask) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PORT(port);
  switch (port) {
  case GPIO_PORT_6:
    GPIO6_DR_TOGGLE = mask;
    break;
  case GPIO_PORT_7:
    GPIO7_DR_TOGGLE = mask;
    break;
  case GPIO_PORT_8:
    GPIO8_DR_TOGGLE = mask;
    break;
  case GPIO_PORT_9:
    GPIO9_DR_TOGGLE = mask;
    break;
  default:
    return -1;
  }
  return 0;
}

/** Adjusts the output drive strength (DSE field) of a configured pin
 * without touching its mode, pull or open-drain configuration.
 * 7 = strongest driver (the default GPIO_OUTPUT config), lower values
 * trade edge rate for reduced EMI/current.
 * @param pin Teensy pin number, previously passed to gpio_configure().
 * @param strength Drive strength 0..7.
 * @return 0 on success, -1 if the pin is not configured or the
 * strength is out of range.
 */
#if defined(__clang__)
static inline int gpio_set_drive_strength(uint8_t pin, uint8_t strength)
    __attribute__((diagnose_if(
        pin >= TEENSY_GPIO_PIN_COUNT,
        "invalid Teensy GPIO pin; expected a value from 0 to 54", "error")));
#endif
static inline __attribute__((always_inline)) int
gpio_set_drive_strength(uint8_t pin, uint8_t strength) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);
  if (strength > 7)
    return -1;
  gpio_pin_t d;
  TEENSY_GPIO_PIN_RESOLVE(d, pin, -1);
  uint32_t pad = *d.pad & ~(uint32_t)IOMUXC_PAD_DSE(7);
  *d.pad = pad | (uint32_t)IOMUXC_PAD_DSE(strength);
  return 0;
}
static inline __attribute__((always_inline)) int
gpio_detach_interrupt(uint8_t pin) {
  TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);
  gpio_pin_t d;
  TEENSY_GPIO_PIN_RESOLVE(d, pin, -1);
  return gpio_detach_interrupt_pin(&d);
}

#endif
