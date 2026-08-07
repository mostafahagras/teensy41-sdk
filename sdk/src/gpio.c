#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <teensy/gpio.h>
#include <teensy/imxrt.h>

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

#define GPIO_PIN(port_number, pin_bit, mux_register, pad_register) \
    { \
        &GPIO##port_number##_DR, \
        &GPIO##port_number##_GDIR, \
        &GPIO##port_number##_PSR, \
        &GPIO##port_number##_DR_SET, \
        &GPIO##port_number##_DR_CLEAR, \
        &GPIO##port_number##_DR_TOGGLE, \
        &(mux_register), \
        &(pad_register), \
        (uint32_t)1u << (pin_bit), \
        (uint8_t)((port_number) - 6u), \
        (uint8_t)(pin_bit) \
    }

static const gpio_pin_t gpio_pins[55] = {
    GPIO_PIN(6, 3, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_03, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_03),
    GPIO_PIN(6, 2, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_02, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_02),
    GPIO_PIN(9, 4, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_04, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_04),
    GPIO_PIN(9, 5, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_05, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_05),
    GPIO_PIN(9, 6, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_06, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_06),
    GPIO_PIN(9, 8, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_08, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_08),
    GPIO_PIN(7, 10, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_10, IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_10),
    GPIO_PIN(7, 17, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_01, IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_01),
    GPIO_PIN(7, 16, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_00, IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_00),
    GPIO_PIN(7, 11, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_11, IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_11),
    GPIO_PIN(7, 0, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_00, IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_00),
    GPIO_PIN(7, 2, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_02, IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_02),
    GPIO_PIN(7, 1, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_01, IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_01),
    GPIO_PIN(7, 3, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_03, IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_03),
    GPIO_PIN(6, 18, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_02, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_02),
    GPIO_PIN(6, 19, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_03, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_03),
    GPIO_PIN(6, 23, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_07, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_07),
    GPIO_PIN(6, 22, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_06, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_06),
    GPIO_PIN(6, 17, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_01, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_01),
    GPIO_PIN(6, 16, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_00, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_00),
    GPIO_PIN(6, 26, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_10, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_10),
    GPIO_PIN(6, 27, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_11, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_11),
    GPIO_PIN(6, 24, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_08, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_08),
    GPIO_PIN(6, 25, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_09, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_09),
    GPIO_PIN(6, 12, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_12, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_12),
    GPIO_PIN(6, 13, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_13, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_13),
    GPIO_PIN(6, 30, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_14, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_14),
    GPIO_PIN(6, 31, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_15, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_15),
    GPIO_PIN(8, 18, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_32, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_32),
    GPIO_PIN(9, 31, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_31, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_31),
    GPIO_PIN(8, 23, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_37, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_37),
    GPIO_PIN(8, 22, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_36, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_36),
    GPIO_PIN(7, 12, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_12, IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_12),
    GPIO_PIN(9, 7, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_07, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_07),
    GPIO_PIN(7, 29, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_13, IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_13),
    GPIO_PIN(7, 28, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_12, IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_12),
    GPIO_PIN(7, 18, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_02, IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_02),
    GPIO_PIN(7, 19, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_03, IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_03),
    GPIO_PIN(6, 28, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_12, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_12),
    GPIO_PIN(6, 29, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_13, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_13),
    GPIO_PIN(6, 20, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_04, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_04),
    GPIO_PIN(6, 21, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_05, IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_05),
    GPIO_PIN(8, 15, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_03, IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_03),
    GPIO_PIN(8, 14, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_02, IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_02),
    GPIO_PIN(8, 13, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_01, IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_01),
    GPIO_PIN(8, 12, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_00, IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_00),
    GPIO_PIN(8, 17, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_05, IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_05),
    GPIO_PIN(8, 16, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_04, IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_04),
    GPIO_PIN(9, 24, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_24, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_24),
    GPIO_PIN(9, 27, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_27, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_27),
    GPIO_PIN(9, 28, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_28, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_28),
    GPIO_PIN(9, 22, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_22, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_22),
    GPIO_PIN(9, 26, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_26, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_26),
    GPIO_PIN(9, 25, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_25, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_25),
    GPIO_PIN(9, 29, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_29, IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_29)
};

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
    {&GPIO9_ISR, &GPIO9_IMR, &GPIO9_EDGE_SEL, &GPIO9_ICR1, &GPIO9_ICR2}
};

static gpio_interrupt_handler_t gpio_handlers[4][32];
static void *gpio_handler_contexts[4][32];

void gpio_irq_handler(void);

static const gpio_pin_t *gpio_pin(uint8_t pin)
{
    return pin < (uint8_t)(sizeof(gpio_pins) / sizeof(gpio_pins[0]))
        ? &gpio_pins[pin]
        : NULL;
}

static uint32_t gpio_pad_for_mode(gpio_mode_t mode)
{
    switch (mode) {
    case GPIO_INPUT_PULLUP:
        return IOMUXC_PAD_DSE(7) | IOMUXC_PAD_PKE | IOMUXC_PAD_PUE
            | IOMUXC_PAD_PUS(3) | IOMUXC_PAD_HYS;
    case GPIO_INPUT_PULLDOWN:
        return IOMUXC_PAD_DSE(7) | IOMUXC_PAD_PKE | IOMUXC_PAD_PUE
            | IOMUXC_PAD_PUS(0) | IOMUXC_PAD_HYS;
    case GPIO_OUTPUT_OPEN_DRAIN:
        return IOMUXC_PAD_DSE(7) | IOMUXC_PAD_ODE;
    case GPIO_INPUT:
    case GPIO_OUTPUT:
        return IOMUXC_PAD_DSE(7);
    default:
        return 0;
    }
}

void gpio_init(void)
{
    attachInterruptVector(IRQ_GPIO6789, gpio_irq_handler);
}

int gpio_configure(uint8_t pin_number, gpio_mode_t mode)
{
    const gpio_pin_t *pin = gpio_pin(pin_number);
    uint32_t pad;

    if (pin == NULL || mode > GPIO_OUTPUT_OPEN_DRAIN) return -1;
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

int gpio_write(uint8_t pin_number, bool high)
{
    const gpio_pin_t *pin = gpio_pin(pin_number);

    if (pin == NULL) return -1;
    if (high) {
        *pin->set = pin->mask;
    } else {
        *pin->clear = pin->mask;
    }
    return 0;
}

int gpio_read(uint8_t pin_number, bool *high)
{
    const gpio_pin_t *pin = gpio_pin(pin_number);

    if (pin == NULL || high == NULL) return -1;
    *high = (*pin->input & pin->mask) != 0;
    return 0;
}

int gpio_toggle(uint8_t pin_number)
{
    const gpio_pin_t *pin = gpio_pin(pin_number);

    if (pin == NULL) return -1;
    *pin->toggle = pin->mask;
    return 0;
}

__attribute__((section(".fastrun")))
void gpio_irq_handler(void)
{
    uint8_t port;

    for (port = 0; port < 4; ++port) {
        const gpio_interrupt_port_t *registers = &gpio_interrupt_ports[port];
        uint32_t pending = *registers->interrupt_status
            & *registers->interrupt_mask;
        *registers->interrupt_status = pending;

        while (pending != 0) {
            uint32_t bit = (uint32_t)__builtin_ctz(pending);
            gpio_interrupt_handler_t handler = gpio_handlers[port][bit];
            void *context = gpio_handler_contexts[port][bit];

            pending &= ~(1u << bit);
            if (handler != NULL) handler(context);
        }
    }
    __asm volatile("dsb" ::: "memory");
}

int gpio_attach_interrupt(uint8_t pin_number,
                          gpio_interrupt_mode_t mode,
                          gpio_interrupt_handler_t handler,
                          void *context)
{
    const gpio_pin_t *pin = gpio_pin(pin_number);
    const gpio_interrupt_port_t *registers;
    uint32_t icr;
    uint32_t shift;

    if (pin == NULL || handler == NULL || mode > GPIO_INTERRUPT_HIGH) {
        return -1;
    }
    if (gpio_configure(pin_number, GPIO_INPUT) != 0) return -1;

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
        *registers->control1 = (*registers->control1 & ~(3u << shift))
            | (icr << shift);
    } else {
        shift = (uint32_t)(pin->bit - 16u) * 2u;
        *registers->control2 = (*registers->control2 & ~(3u << shift))
            | (icr << shift);
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

int gpio_detach_interrupt(uint8_t pin_number)
{
    const gpio_pin_t *pin = gpio_pin(pin_number);
    const gpio_interrupt_port_t *registers;

    if (pin == NULL) return -1;
    registers = &gpio_interrupt_ports[pin->port];
    __disable_irq();
    *registers->interrupt_mask &= ~pin->mask;
    gpio_handlers[pin->port][pin->bit] = NULL;
    gpio_handler_contexts[pin->port][pin->bit] = NULL;
    __enable_irq();
    return 0;
}
