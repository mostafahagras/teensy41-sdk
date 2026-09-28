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

/* Restores the interrupt state as it was, instead of blindly re-enabling
 * interrupts from inside a critical section. */
static uint32_t gpio_critical_enter(void) {
  uint32_t primask;

  __asm volatile("mrs %0, primask\n"
                 "cpsid i\n"
                 : "=r"(primask)
                 :
                 : "memory");
  return primask;
}

static void gpio_critical_leave(uint32_t primask) {
  if ((primask & 1u) == 0)
    __enable_irq();
}

void gpio_init(void) { attachInterruptVector(IRQ_GPIO6789, gpio_irq_handler); }

uint8_t gpio_configured_mask[4];

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
  uint32_t primask;
  uint32_t icr;
  uint32_t shift;

  if (pin == NULL || handler == NULL || mode > GPIO_INTERRUPT_HIGH) {
    return -1;
  }
  if ((gpio_configured_mask[(uint8_t)(pin->port)] &
       (uint8_t)(1u << pin->bit)) == 0) {
    return -1; /* attach only works on pins gpio_configure() has set up */
  }

  registers = &gpio_interrupt_ports[pin->port];
  primask = gpio_critical_enter();
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
    gpio_critical_leave(primask);
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
  gpio_critical_leave(primask);
  return 0;
}

int gpio_detach_interrupt_pin(const gpio_pin_t *pin) {
  const gpio_interrupt_port_t *registers;
  uint32_t primask;

  if (pin == NULL)
    return -1;
  registers = &gpio_interrupt_ports[pin->port];
  primask = gpio_critical_enter();
  *registers->interrupt_mask &= ~pin->mask;
  gpio_handlers[pin->port][pin->bit] = NULL;
  gpio_handler_contexts[pin->port][pin->bit] = NULL;
  gpio_critical_leave(primask);
  return 0;
}
