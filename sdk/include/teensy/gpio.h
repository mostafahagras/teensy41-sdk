#ifndef TEENSY_GPIO_H
#define TEENSY_GPIO_H

#include <stdbool.h>
#include <stdint.h>

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

#endif
