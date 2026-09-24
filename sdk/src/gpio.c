#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TEENSY_GPIO_IMPLEMENTATION
#include <teensy/gpio.h>
#include <teensy/imxrt.h>

#define GPIO_PIN(port_number, pin_bit, mux_register, pad_register)             \
  TEENSY_GPIO_PIN_DESCRIPTOR(port_number, pin_bit, mux_register, pad_register)

#define GPIO_PIN_ENTRY(number, port_number, pin_bit, mux_register,             \
                       pad_register)                                           \
  GPIO_PIN(port_number, pin_bit, mux_register, pad_register),

static const gpio_pin_t gpio_pins[TEENSY_GPIO_PIN_COUNT] = {
    TEENSY_GPIO_PIN_MAP(GPIO_PIN_ENTRY)};

#undef GPIO_PIN_ENTRY

typedef struct {
  volatile uint32_t *interrupt_status;
  volatile uint32_t *interrupt_mask;
  volatile uint32_t *edge;
  volatile uint32_t *control1;
  volatile uint32_t *control2;
} gpio_interrupt_port_t;

static const gpio_interrupt_port_t gpio_interrupt_ports[4] = {
    {&GPIO6_ISR, &GPIO6_IMR, &GPIO6_EDGE_SEL, &GPIO6_ICR1, &GPIO6_ICR2},
    {&GPIO7_ISR, &GPIO7_IMR, &GPIO7_EDGE_SEL, &GPIO7_ICR1, &GPIO7_ICR2},
    {&GPIO8_ISR, &GPIO8_IMR, &GPIO8_EDGE_SEL, &GPIO8_ICR1, &GPIO8_ICR2},
    {&GPIO9_ISR, &GPIO9_IMR, &GPIO9_EDGE_SEL, &GPIO9_ICR1, &GPIO9_ICR2}};

static gpio_interrupt_handler_t gpio_handlers[4][32];
static void *gpio_handler_contexts[4][32];

void gpio_irq_handler(void);

static const gpio_pin_t *gpio_pin(uint8_t pin) {
  return pin < (uint8_t)(sizeof(gpio_pins) / sizeof(gpio_pins[0]))
             ? &gpio_pins[pin]
             : NULL;
}

const gpio_pin_t *gpio_pin_runtime(uint8_t pin) { return gpio_pin(pin); }

void gpio_init(void) { attachInterruptVector(IRQ_GPIO6789, gpio_irq_handler); }

int gpio_configure(uint8_t pin_number, gpio_mode_t mode) {
  const gpio_pin_t *pin = gpio_pin(pin_number);
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

int gpio_write(uint8_t pin_number, bool high) {
  const gpio_pin_t *pin = gpio_pin(pin_number);

  if (pin == NULL)
    return -1;
  if (high) {
    *pin->set = pin->mask;
  } else {
    *pin->clear = pin->mask;
  }
  return 0;
}

int gpio_read(uint8_t pin_number, bool *high) {
  const gpio_pin_t *pin = gpio_pin(pin_number);

  if (pin == NULL || high == NULL)
    return -1;
  *high = (*pin->input & pin->mask) != 0;
  return 0;
}

int gpio_toggle(uint8_t pin_number) {
  const gpio_pin_t *pin = gpio_pin(pin_number);

  if (pin == NULL)
    return -1;
  *pin->toggle = pin->mask;
  return 0;
}

__attribute__((section(".fastrun"))) void gpio_irq_handler(void) {
  uint8_t port;

  for (port = 0; port < 4; ++port) {
    const gpio_interrupt_port_t *registers = &gpio_interrupt_ports[port];
    uint32_t pending =
        *registers->interrupt_status & *registers->interrupt_mask;
    *registers->interrupt_status = pending;

    while (pending != 0) {
      uint32_t bit = (uint32_t)__builtin_ctz(pending);
      gpio_interrupt_handler_t handler = gpio_handlers[port][bit];
      void *context = gpio_handler_contexts[port][bit];

      pending &= ~(1u << bit);
      if (handler != NULL)
        handler(context);
    }
  }
  __asm volatile("dsb" ::: "memory");
}

int gpio_attach_interrupt_pin(const gpio_pin_t *pin, gpio_interrupt_mode_t mode,
                              gpio_interrupt_handler_t handler, void *context) {
  const gpio_interrupt_port_t *registers;
  uint32_t icr;
  uint32_t shift;

  if (pin == NULL || handler == NULL || mode > GPIO_INTERRUPT_HIGH) {
    return -1;
  }
  if (gpio_configure_pin(pin, GPIO_INPUT) != 0)
    return -1;

  registers = &gpio_interrupt_ports[pin->port];
  __disable_irq();
  *registers->interrupt_mask &= ~pin->mask;
  gpio_handlers[pin->port][pin->bit] = handler;
  gpio_handler_contexts[pin->port][pin->bit] = context;

  switch (mode) {
  case GPIO_INTERRUPT_CHANGE:
    *registers->edge |= pin->mask;
    break;
  case GPIO_INTERRUPT_FALLING:
    icr = 3;
    *registers->edge &= ~pin->mask;
    goto configure_level;
  case GPIO_INTERRUPT_RISING:
    icr = 2;
    *registers->edge &= ~pin->mask;
    goto configure_level;
  case GPIO_INTERRUPT_LOW:
    icr = 0;
    *registers->edge &= ~pin->mask;
    goto configure_level;
  case GPIO_INTERRUPT_HIGH:
    icr = 1;
    *registers->edge &= ~pin->mask;
    goto configure_level;
  default:
    __enable_irq();
    return -1;
  }
  goto enable_interrupt;

configure_level:
  if (pin->bit < 16) {
    shift = (uint32_t)pin->bit * 2u;
    *registers->control1 =
        (*registers->control1 & ~(3u << shift)) | (icr << shift);
  } else {
    shift = (uint32_t)(pin->bit - 16u) * 2u;
    *registers->control2 =
        (*registers->control2 & ~(3u << shift)) | (icr << shift);
  }

enable_interrupt:
  *registers->interrupt_status = pin->mask;
  attachInterruptVector(IRQ_GPIO6789, gpio_irq_handler);
  NVIC_SET_PRIORITY(IRQ_GPIO6789, 128);
  NVIC_ENABLE_IRQ(IRQ_GPIO6789);
  *registers->interrupt_mask |= pin->mask;
  __enable_irq();
  return 0;
}

int gpio_attach_interrupt(uint8_t pin_number, gpio_interrupt_mode_t mode,
                          gpio_interrupt_handler_t handler, void *context) {
  const gpio_pin_t *pin = gpio_pin(pin_number);
  return gpio_attach_interrupt_pin(pin, mode, handler, context);
}

int gpio_detach_interrupt_pin(const gpio_pin_t *pin) {
  const gpio_interrupt_port_t *registers;

  if (pin == NULL)
    return -1;
  registers = &gpio_interrupt_ports[pin->port];
  __disable_irq();
  *registers->interrupt_mask &= ~pin->mask;
  gpio_handlers[pin->port][pin->bit] = NULL;
  gpio_handler_contexts[pin->port][pin->bit] = NULL;
  __enable_irq();
  return 0;
}

int gpio_detach_interrupt(uint8_t pin_number) {
  const gpio_pin_t *pin = gpio_pin(pin_number);
  return gpio_detach_interrupt_pin(pin);
}
