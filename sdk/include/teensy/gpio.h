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

void gpio_init(void);

int gpio_configure(uint8_t pin, gpio_mode_t mode);
int gpio_write(uint8_t pin, bool high);
int gpio_read(uint8_t pin, bool *high);
int gpio_toggle(uint8_t pin);

int gpio_attach_interrupt(uint8_t pin, gpio_interrupt_mode_t mode,
                          gpio_interrupt_handler_t handler, void *context);
int gpio_detach_interrupt(uint8_t pin);

#endif
