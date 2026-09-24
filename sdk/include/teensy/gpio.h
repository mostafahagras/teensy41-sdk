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

/** Initializes GPIO interrupt handling. */
void gpio_init(void);

/** Configures a Teensy pin for the specified GPIO mode.
 * @return 0 on success, or -1 if the pin or mode is invalid.
 */
int gpio_configure(uint8_t pin, gpio_mode_t mode);

/** Drives a GPIO pin high or low.
 * @return 0 on success, or -1 if the pin is invalid.
 */
int gpio_write(uint8_t pin, bool high);

/** Stores the current logic level of a GPIO pin in @p high.
 * @return 0 on success, or -1 if the pin or output pointer is invalid.
 */
int gpio_read(uint8_t pin, bool *high);

/** Inverts the output latch of a GPIO pin.
 * @return 0 on success, or -1 if the pin is invalid.
 */
int gpio_toggle(uint8_t pin);

/** Registers a callback for changes on a GPIO pin.
 *
 * The callback runs in interrupt context and receives @p context unchanged.
 * Configuring an interrupt also configures the pin as a plain input.
 *
 * @return 0 on success, or -1 if the pin, mode, or handler is invalid.
 */
int gpio_attach_interrupt(uint8_t pin, gpio_interrupt_mode_t mode,
                          gpio_interrupt_handler_t handler, void *context);

/** Disables and removes the interrupt callback for a GPIO pin.
 * @return 0 on success, or -1 if the pin is invalid.
 */
int gpio_detach_interrupt(uint8_t pin);

/* Internal descriptor entry point used by compile-time pin wrappers. */
const gpio_pin_t *gpio_pin_runtime(uint8_t pin);
int gpio_attach_interrupt_pin(const gpio_pin_t *pin, gpio_interrupt_mode_t mode,
                              gpio_interrupt_handler_t handler, void *context);
int gpio_detach_interrupt_pin(const gpio_pin_t *pin);

static inline __attribute__((always_inline)) gpio_pin_t
gpio_pin_const(uint8_t pin) {
#define TEENSY_GPIO_PIN_CASE(number, port_number, pin_bit, mux_register,       \
                             pad_register)                                     \
  case number:                                                                 \
    return (gpio_pin_t){&GPIO##port_number##_DR,                               \
                        &GPIO##port_number##_GDIR,                             \
                        &GPIO##port_number##_PSR,                              \
                        &GPIO##port_number##_DR_SET,                           \
                        &GPIO##port_number##_DR_CLEAR,                         \
                        &GPIO##port_number##_DR_TOGGLE,                        \
                        &(mux_register),                                       \
                        &(pad_register),                                       \
                        (uint32_t)1u << (pin_bit),                             \
                        (uint8_t)((port_number) - 6u),                         \
                        (uint8_t)(pin_bit)};

  switch (pin) {
    TEENSY_GPIO_PIN_MAP(TEENSY_GPIO_PIN_CASE)
  default:
    return (gpio_pin_t){0};
  }
#undef TEENSY_GPIO_PIN_CASE
}

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

  if (mode > GPIO_OUTPUT_OPEN_DRAIN)
    return -1;
  pad = gpio_pad_for_mode(mode);
  if (mode == GPIO_OUTPUT || mode == GPIO_OUTPUT_OPEN_DRAIN)
    *pin->direction |= pin->mask;
  else
    *pin->direction &= ~pin->mask;
  *pin->pad = pad;
  *pin->mux = 5u | 0x10u;
  return 0;
}

static inline __attribute__((always_inline)) int
gpio_configure_const(uint8_t pin, gpio_mode_t mode) {
  gpio_pin_t descriptor = gpio_pin_const(pin);
  return gpio_configure_pin(&descriptor, mode);
}

static inline __attribute__((always_inline)) int gpio_write_const(uint8_t pin,
                                                                  bool high) {
  gpio_pin_t descriptor = gpio_pin_const(pin);
  if (high)
    *descriptor.set = descriptor.mask;
  else
    *descriptor.clear = descriptor.mask;
  return 0;
}

static inline __attribute__((always_inline)) int gpio_read_const(uint8_t pin,
                                                                 bool *high) {
  gpio_pin_t descriptor = gpio_pin_const(pin);
  if (high == NULL)
    return -1;
  *high = (*descriptor.input & descriptor.mask) != 0;
  return 0;
}

static inline __attribute__((always_inline)) int
gpio_toggle_const(uint8_t pin) {
  gpio_pin_t descriptor = gpio_pin_const(pin);
  *descriptor.toggle = descriptor.mask;
  return 0;
}

static inline __attribute__((always_inline)) int
gpio_attach_interrupt_const(uint8_t pin, gpio_interrupt_mode_t mode,
                            gpio_interrupt_handler_t handler, void *context) {
  gpio_pin_t descriptor = gpio_pin_const(pin);
  return gpio_attach_interrupt_pin(&descriptor, mode, handler, context);
}

static inline __attribute__((always_inline)) int
gpio_detach_interrupt_const(uint8_t pin) {
  gpio_pin_t descriptor = gpio_pin_const(pin);
  return gpio_detach_interrupt_pin(&descriptor);
}

#ifndef TEENSY_GPIO_IMPLEMENTATION
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

#define gpio_configure(pin, mode)                                              \
  ({                                                                           \
    TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);                                    \
    __builtin_choose_expr(__builtin_constant_p(pin),                           \
                          gpio_configure_const((uint8_t)(pin), (mode)),        \
                          gpio_configure((uint8_t)(pin), (mode)));             \
  })

#define gpio_write(pin, high)                                                  \
  ({                                                                           \
    TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);                                    \
    __builtin_choose_expr(__builtin_constant_p(pin),                           \
                          gpio_write_const((uint8_t)(pin), (high)),            \
                          gpio_write((uint8_t)(pin), (high)));                 \
  })

#define gpio_read(pin, high)                                                   \
  ({                                                                           \
    TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);                                    \
    __builtin_choose_expr(__builtin_constant_p(pin),                           \
                          gpio_read_const((uint8_t)(pin), (high)),             \
                          gpio_read((uint8_t)(pin), (high)));                  \
  })

#define gpio_toggle(pin)                                                       \
  ({                                                                           \
    TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);                                    \
    __builtin_choose_expr(__builtin_constant_p(pin),                           \
                          gpio_toggle_const((uint8_t)(pin)),                   \
                          gpio_toggle((uint8_t)(pin)));                        \
  })

#define gpio_attach_interrupt(pin, mode, handler, context)                     \
  ({                                                                           \
    TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);                                    \
    __builtin_choose_expr(                                                     \
        __builtin_constant_p(pin),                                             \
        gpio_attach_interrupt_const((uint8_t)(pin), (mode), (handler),         \
                                    (context)),                                \
        gpio_attach_interrupt((uint8_t)(pin), (mode), (handler), (context)));  \
  })

#define gpio_detach_interrupt(pin)                                             \
  ({                                                                           \
    TEENSY_GPIO_VALIDATE_CONSTANT_PIN(pin);                                    \
    __builtin_choose_expr(__builtin_constant_p(pin),                           \
                          gpio_detach_interrupt_const((uint8_t)(pin)),         \
                          gpio_detach_interrupt((uint8_t)(pin)));              \
  })
#endif

#endif
