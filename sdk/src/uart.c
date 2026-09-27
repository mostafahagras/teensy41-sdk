#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <teensy/clock.h>
#include <teensy/gpio.h>
#include <teensy/imxrt.h>
#include <teensy/time.h>
#define TEENSY_UART_IMPLEMENTATION
#include <teensy/uart.h>

#define UART_RX_CAPACITY 256u
#define UART_TX_CAPACITY 256u
#define UART_IRQ_PRIORITY 64u

typedef struct {
  IMXRT_LPUART_t *port;
  volatile uint32_t *clock_gate;
  uint32_t clock_gate_mask;
  enum IRQ_NUMBER_t irq;
  void (*irq_handler)(void);
  gpio_pin_t rx_gpio;
  gpio_pin_t tx_gpio;
  volatile uint32_t *rx_mux;
  volatile uint32_t *rx_pad;
  volatile uint32_t *rx_select;
  uint32_t rx_mux_value;
  uint32_t rx_select_value;
  volatile uint32_t *tx_mux;
  volatile uint32_t *tx_pad;
  volatile uint32_t *tx_select;
  uint32_t tx_mux_value;
  uint32_t tx_select_value;
} uart_config_t;

typedef struct {
  uint8_t rx_buffer[UART_RX_CAPACITY];
  uint8_t tx_buffer[UART_TX_CAPACITY];
  volatile uint16_t rx_head;
  volatile uint16_t rx_tail;
  volatile uint16_t tx_head;
  volatile uint16_t tx_tail;
  void (*rx_callback)(uint8_t byte, void *context);
  void (*rx_idle_callback)(void *context);
  void *rx_context;
  void *rx_idle_context;
  bool initialized;
} uart_state_t;

struct uart_device {
  const uart_config_t *config;
  uart_state_t *state;
};

static uart_state_t uart_state6;
static uart_state_t uart_state3;
static uart_state_t uart_state4;
static uart_state_t uart_state2;
static uart_state_t uart_state8;
static uart_state_t uart_state1;
static uart_state_t uart_state7;
static uart_state_t uart_state5;

static void uart_irq_handler(const uart_device_t *device);
static void uart_irq_handler6(void);
static void uart_irq_handler3(void);
static void uart_irq_handler4(void);
static void uart_irq_handler2(void);
static void uart_irq_handler8(void);
static void uart_irq_handler1(void);
static void uart_irq_handler7(void);
static void uart_irq_handler5(void);

static const uart_config_t uart_config6 = {
    &IMXRT_LPUART1,
    &CCM_CCGR5,
    CCM_CCGR5_LPUART1(CCM_CCGR_ON),
    IRQ_LPUART1,
    uart_irq_handler6,
    TEENSY_GPIO_PIN_DESCRIPTOR(6, 13, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_13,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_13),
    TEENSY_GPIO_PIN_DESCRIPTOR(6, 12, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_12,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_12),
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_13,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_13,
    NULL,
    2,
    0,
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_12,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_12,
    NULL,
    2,
    0};
static const uart_config_t uart_config3 = {
    &IMXRT_LPUART2,
    &CCM_CCGR0,
    CCM_CCGR0_LPUART2(CCM_CCGR_ON),
    IRQ_LPUART2,
    uart_irq_handler3,
    TEENSY_GPIO_PIN_DESCRIPTOR(6, 19, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_03,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_03),
    TEENSY_GPIO_PIN_DESCRIPTOR(6, 18, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_02,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_02),
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_03,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_03,
    &IOMUXC_LPUART2_RX_SELECT_INPUT,
    2,
    1,
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_02,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_02,
    &IOMUXC_LPUART2_TX_SELECT_INPUT,
    2,
    1};
static const uart_config_t uart_config4 = {
    &IMXRT_LPUART3,
    &CCM_CCGR0,
    CCM_CCGR0_LPUART3(CCM_CCGR_ON),
    IRQ_LPUART3,
    uart_irq_handler4,
    TEENSY_GPIO_PIN_DESCRIPTOR(6, 23, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_07,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_07),
    TEENSY_GPIO_PIN_DESCRIPTOR(6, 22, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_06,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_06),
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_07,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_07,
    &IOMUXC_LPUART3_RX_SELECT_INPUT,
    2,
    0,
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_06,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_06,
    &IOMUXC_LPUART3_TX_SELECT_INPUT,
    2,
    0};
static const uart_config_t uart_config2 = {
    &IMXRT_LPUART4,
    &CCM_CCGR1,
    CCM_CCGR1_LPUART4(CCM_CCGR_ON),
    IRQ_LPUART4,
    uart_irq_handler2,
    TEENSY_GPIO_PIN_DESCRIPTOR(7, 17, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_01,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_01),
    TEENSY_GPIO_PIN_DESCRIPTOR(7, 16, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_00,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_00),
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_01,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_01,
    &IOMUXC_LPUART4_RX_SELECT_INPUT,
    2,
    2,
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_00,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_00,
    &IOMUXC_LPUART4_TX_SELECT_INPUT,
    2,
    2};
static const uart_config_t uart_config8 = {
    &IMXRT_LPUART5,
    &CCM_CCGR3,
    CCM_CCGR3_LPUART5(CCM_CCGR_ON),
    IRQ_LPUART5,
    uart_irq_handler8,
    TEENSY_GPIO_PIN_DESCRIPTOR(7, 29, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_13,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_13),
    TEENSY_GPIO_PIN_DESCRIPTOR(7, 28, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_12,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_12),
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_13,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_13,
    &IOMUXC_LPUART5_RX_SELECT_INPUT,
    1,
    1,
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_12,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_12,
    NULL,
    1,
    0};
static const uart_config_t uart_config1 = {
    &IMXRT_LPUART6,
    &CCM_CCGR3,
    CCM_CCGR3_LPUART6(CCM_CCGR_ON),
    IRQ_LPUART6,
    uart_irq_handler1,
    TEENSY_GPIO_PIN_DESCRIPTOR(6, 3, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_03,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_03),
    TEENSY_GPIO_PIN_DESCRIPTOR(6, 2, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_02,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_02),
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_03,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_03,
    &IOMUXC_LPUART6_RX_SELECT_INPUT,
    2,
    1,
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_02,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_02,
    &IOMUXC_LPUART6_TX_SELECT_INPUT,
    2,
    0};
static const uart_config_t uart_config7 = {
    &IMXRT_LPUART7,
    &CCM_CCGR5,
    CCM_CCGR5_LPUART7(CCM_CCGR_ON),
    IRQ_LPUART7,
    uart_irq_handler7,
    TEENSY_GPIO_PIN_DESCRIPTOR(8, 18, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_32,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_32),
    TEENSY_GPIO_PIN_DESCRIPTOR(9, 31, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_31,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_31),
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_32,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_32,
    &IOMUXC_LPUART7_RX_SELECT_INPUT,
    2,
    1,
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_31,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_31,
    &IOMUXC_LPUART7_TX_SELECT_INPUT,
    2,
    0};
static const uart_config_t uart_config5 = {
    &IMXRT_LPUART8,
    &CCM_CCGR6,
    CCM_CCGR6_LPUART8(CCM_CCGR_ON),
    IRQ_LPUART8,
    uart_irq_handler5,
    TEENSY_GPIO_PIN_DESCRIPTOR(6, 27, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_11,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_11),
    TEENSY_GPIO_PIN_DESCRIPTOR(6, 26, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_10,
                               IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_10),
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_11,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_11,
    &IOMUXC_LPUART8_RX_SELECT_INPUT,
    2,
    1,
    &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_10,
    &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_10,
    &IOMUXC_LPUART8_TX_SELECT_INPUT,
    2,
    1};

uart_device_t uart_device6 = {&uart_config6, &uart_state6};
uart_device_t uart_device3 = {&uart_config3, &uart_state3};
uart_device_t uart_device4 = {&uart_config4, &uart_state4};
uart_device_t uart_device2 = {&uart_config2, &uart_state2};
uart_device_t uart_device8 = {&uart_config8, &uart_state8};
uart_device_t uart_device1 = {&uart_config1, &uart_state1};
uart_device_t uart_device7 = {&uart_config7, &uart_state7};
uart_device_t uart_device5 = {&uart_config5, &uart_state5};

static uart_device_t *const uart_devices[UART_COUNT] = {
    [UART_ID_1] = &uart_device6, [UART_ID_2] = &uart_device3,
    [UART_ID_3] = &uart_device4, [UART_ID_4] = &uart_device2,
    [UART_ID_5] = &uart_device8, [UART_ID_6] = &uart_device1,
    [UART_ID_7] = &uart_device7, [UART_ID_8] = &uart_device5};

static void uart_irq_handler6(void) { uart_irq_handler(&uart_device6); }
static void uart_irq_handler3(void) { uart_irq_handler(&uart_device3); }
static void uart_irq_handler4(void) { uart_irq_handler(&uart_device4); }
static void uart_irq_handler2(void) { uart_irq_handler(&uart_device2); }
static void uart_irq_handler8(void) { uart_irq_handler(&uart_device8); }
static void uart_irq_handler1(void) { uart_irq_handler(&uart_device1); }
static void uart_irq_handler7(void) { uart_irq_handler(&uart_device7); }
static void uart_irq_handler5(void) { uart_irq_handler(&uart_device5); }

static bool uart_valid(uart_id_t uart) {
  return uart >= UART_ID_1 && uart <= UART_ID_8;
}

static uint32_t uart_critical_enter(void) {
  uint32_t primask;

  __asm volatile("mrs %0, primask\n"
                 "cpsid i\n"
                 : "=r"(primask)
                 :
                 : "memory");
  return primask;
}

static void uart_critical_leave(uint32_t primask) {
  if ((primask & 1u) == 0)
    __enable_irq();
}

/* Finds the best over-sampling/divisor pair for @p baud_rate and returns
 * the achieved rate through @p actual. */
static uint32_t uart_best_baud(uint32_t baud_rate, uint32_t *osr_out,
                               uint32_t *sbr_out) {
  uint32_t best_osr = 4;
  uint32_t best_sbr = 1;
  uint32_t best_error = UINT32_MAX;
  uint32_t osr;

  if (baud_rate == 0)
    baud_rate = 1;
  for (osr = 4; osr <= 32; ++osr) {
    uint32_t divisor =
        (clock_uart_frequency_hz + (baud_rate * osr) / 2u) / (baud_rate * osr);
    uint32_t actual_baud;
    uint32_t error;

    if (divisor < 1)
      divisor = 1;
    if (divisor > 8191)
      divisor = 8191;
    actual_baud = clock_uart_frequency_hz / (osr * divisor);
    error = actual_baud > baud_rate ? actual_baud - baud_rate
                                    : baud_rate - actual_baud;
    if (error < best_error) {
      best_error = error;
      best_osr = osr;
      best_sbr = divisor;
    }
  }

  *osr_out = best_osr;
  *sbr_out = best_sbr;
  return clock_uart_frequency_hz / (best_osr * best_sbr);
}

static uint32_t uart_baud_register(uint32_t baud_rate) {
  uint32_t osr = 0;
  uint32_t sbr = 0;
  uint32_t actual;

  actual = uart_best_baud(baud_rate, &osr, &sbr);
  (void)actual;
  return LPUART_BAUD_OSR(osr - 1u) | LPUART_BAUD_SBR(sbr) |
         (osr <= 8u ? LPUART_BAUD_BOTHEDGE : 0u);
}

static bool uart_tx_empty(const uart_state_t *state) {
  return state->tx_head == state->tx_tail;
}

static void uart_irq_handler(const uart_device_t *device) {
  const uart_config_t *config = device->config;
  uart_state_t *state = device->state;
  IMXRT_LPUART_t *port = config->port;
  uint32_t status = port->STAT;
  uint32_t control = port->CTRL;

  if (status &
      (LPUART_STAT_OR | LPUART_STAT_NF | LPUART_STAT_FE | LPUART_STAT_PF)) {
    port->STAT =
        LPUART_STAT_OR | LPUART_STAT_NF | LPUART_STAT_FE | LPUART_STAT_PF;
  }

  while (status & LPUART_STAT_RDRF) {
    uint16_t next = (uint16_t)((state->rx_head + 1u) % UART_RX_CAPACITY);
    uint8_t byte = (uint8_t)(port->DATA & 0xFFu);

    if (next != state->rx_tail) {
      state->rx_buffer[state->rx_head] = byte;
      state->rx_head = next;
      if (state->rx_callback)
        state->rx_callback(byte, state->rx_context);
    }
    status = port->STAT;
  }

  if (state->rx_idle_callback && (control & LPUART_CTRL_ILIE) &&
      (status & LPUART_STAT_IDLE)) {
    port->STAT = LPUART_STAT_IDLE;
    state->rx_idle_callback(state->rx_idle_context);
  }

  if ((control & LPUART_CTRL_TIE) && (status & LPUART_STAT_TDRE)) {
    while (!uart_tx_empty(state) && (port->STAT & LPUART_STAT_TDRE)) {
      port->DATA = state->tx_buffer[state->tx_tail];
      state->tx_tail = (uint16_t)((state->tx_tail + 1u) % UART_TX_CAPACITY);
    }
    if (uart_tx_empty(state)) {
      port->CTRL = (port->CTRL & ~LPUART_CTRL_TIE) | LPUART_CTRL_TCIE;
    }
  }

  if ((control & LPUART_CTRL_TCIE) && (status & LPUART_STAT_TC)) {
    port->CTRL &= ~LPUART_CTRL_TCIE;
  }
}

void uart_attach_rx_impl(uart_id_t uart,
                         void (*callback)(uint8_t byte, void *context),
                         void *user_context) {
  if (!uart_valid(uart))
    return;
  uart_state_t *state = uart_devices[uart]->state;
  uint32_t primask = uart_critical_enter();
  state->rx_callback = callback;
  state->rx_context = user_context;
  uart_critical_leave(primask);
}

void uart_attach_rx_idle_impl(uart_id_t uart, void (*callback)(void *context),
                              void *user_context) {
  if (!uart_valid(uart))
    return;
  const uart_device_t *device = uart_devices[uart];
  volatile IMXRT_LPUART_t *port = device->config->port;
  uint32_t primask = uart_critical_enter();
  device->state->rx_idle_callback = callback;
  device->state->rx_idle_context = user_context;
  port->CTRL = callback ? (port->CTRL | LPUART_CTRL_ILIE)
                        : (port->CTRL & ~LPUART_CTRL_ILIE);
  uart_critical_leave(primask);
}

int uart_set_format_device(uart_device_t *device, uint8_t data_bits,
                           uint8_t stop_bits, uart_parity_t parity) {
  uart_state_t *state = device->state;
  volatile IMXRT_LPUART_t *port = device->config->port;
  uint32_t ctrl;
  uint32_t baud;

  if (!state->initialized)
    return -1;
  /* LPUART framing is decided in data-bit/parity pairs: 8 bits without
   * parity, 8 bits + 1 parity with M set, or 7 bits + 1 parity without M.
   * 5/6-bit frames and 7 bits without parity are not expressible. */
  if (data_bits != 7 && data_bits != 8)
    return -1;
  if (data_bits == 7 && parity == UART_PARITY_NONE)
    return -1;
  if (stop_bits < 1 || stop_bits > 2)
    return -1;

  uint32_t primask = uart_critical_enter();
  port->CTRL &= ~(LPUART_CTRL_TE | LPUART_CTRL_RE);
  while (!(port->STAT & LPUART_STAT_TC))
    ;
  ctrl = port->CTRL;
  ctrl &= ~(LPUART_CTRL_M | LPUART_CTRL_PE | LPUART_CTRL_PT);
  if (parity != UART_PARITY_NONE)
    ctrl |= LPUART_CTRL_PE;
  if (parity == UART_PARITY_ODD)
    ctrl |= LPUART_CTRL_PT;
  if (data_bits == 8 && parity != UART_PARITY_NONE)
    ctrl |= LPUART_CTRL_M;
  port->CTRL = ctrl;
  baud = port->BAUD & ~LPUART_BAUD_SBNS;
  if (stop_bits == 2)
    baud |= LPUART_BAUD_SBNS;
  port->BAUD = baud;
  port->CTRL |= LPUART_CTRL_TE | LPUART_CTRL_RE;
  uart_critical_leave(primask);
  return 0;
}

uint32_t uart_set_baud_device(uart_device_t *device, uint32_t baud_rate) {
  uart_state_t *state = device->state;
  volatile IMXRT_LPUART_t *port = device->config->port;
  uint32_t osr = 0;
  uint32_t sbr = 0;
  uint32_t actual;
  uint32_t ctrl;

  if (!state->initialized || baud_rate == 0)
    return 0;
  actual = uart_best_baud(baud_rate, &osr, &sbr);
  uint32_t primask = uart_critical_enter();
  ctrl = port->CTRL;
  port->CTRL = ctrl & ~(LPUART_CTRL_TE | LPUART_CTRL_RE);
  while (!(port->STAT & LPUART_STAT_TC))
    ;
  port->BAUD = LPUART_BAUD_OSR(osr - 1u) | LPUART_BAUD_SBR(sbr) |
               (osr <= 8u ? LPUART_BAUD_BOTHEDGE : 0u);
  port->CTRL = ctrl;
  uart_critical_leave(primask);
  return actual;
}

void uart_clear_device(uart_device_t *device) {
  uart_state_t *state = device->state;
  uint32_t primask;

  primask = uart_critical_enter();
  state->rx_tail = state->rx_head;
  uart_critical_leave(primask);
}

int uart_available_for_write_device(uart_device_t *device) {
  uart_state_t *state = device->state;
  uint32_t primask;
  int free_bytes;

  if (!state->initialized)
    return 0;
  primask = uart_critical_enter();
  free_bytes = (int)(UART_TX_CAPACITY + state->tx_tail - state->tx_head - 1u) %
               (int)UART_TX_CAPACITY;
  uart_critical_leave(primask);
  return free_bytes;
}

bool uart_is_readable_within_us_device(uart_device_t *device, uint32_t us) {
  uart_state_t *state = device->state;
  uint32_t start;

  if (!state->initialized)
    return false;
  start = time_micros();
  while ((uint32_t)(time_micros() - start) < us) {
    if (state->rx_head != state->rx_tail)
      return true;
  }
  return state->rx_head != state->rx_tail;
}

int uart_init_device(uart_device_t *device, uint32_t baud_rate) {
  const uart_config_t *config = device->config;
  uart_state_t *state = device->state;
  IMXRT_LPUART_t *port;
  uint32_t primask;

  port = config->port;

  if (gpio_configure_pin(&config->rx_gpio, GPIO_INPUT) != 0 ||
      gpio_configure_pin(&config->tx_gpio, GPIO_OUTPUT) != 0) {
    return -1;
  }

  primask = uart_critical_enter();
  state->rx_head = 0;
  state->rx_tail = 0;
  state->tx_head = 0;
  state->tx_tail = 0;
  state->initialized = false;

  *config->clock_gate |= config->clock_gate_mask;
  port->CTRL = 0;
  port->GLOBAL = LPUART_GLOBAL_RST;
  port->GLOBAL = 0;

  *config->rx_pad = IOMUXC_PAD_DSE(7) | IOMUXC_PAD_PKE | IOMUXC_PAD_PUE |
                    IOMUXC_PAD_PUS(3) | IOMUXC_PAD_HYS;
  *config->rx_mux = config->rx_mux_value;
  if (config->rx_select != NULL)
    *config->rx_select = config->rx_select_value;

  *config->tx_pad = IOMUXC_PAD_SRE | IOMUXC_PAD_DSE(3) | IOMUXC_PAD_SPEED(3);
  *config->tx_mux = config->tx_mux_value;
  if (config->tx_select != NULL)
    *config->tx_select = config->tx_select_value;

  port->BAUD = uart_baud_register(baud_rate);
  port->FIFO = LPUART_FIFO_TXFE | LPUART_FIFO_RXFE;
  port->WATER = LPUART_WATER_RXWATER(0) | LPUART_WATER_TXWATER(0);
  port->STAT = LPUART_STAT_OR | LPUART_STAT_NF | LPUART_STAT_FE |
               LPUART_STAT_PF | LPUART_STAT_IDLE;
  port->CTRL = LPUART_CTRL_TE | LPUART_CTRL_RE | LPUART_CTRL_RIE;

  attachInterruptVector(config->irq, config->irq_handler);
  NVIC_CLEAR_PENDING(config->irq);
  NVIC_SET_PRIORITY(config->irq, UART_IRQ_PRIORITY);
  NVIC_ENABLE_IRQ(config->irq);
  state->initialized = true;
  uart_critical_leave(primask);
  return 0;
}

int uart_available_device(uart_device_t *device) {
  uart_state_t *state = device->state;
  uint32_t primask;
  uint16_t head;
  uint16_t tail;

  if (!state->initialized)
    return 0;
  primask = uart_critical_enter();
  head = state->rx_head;
  tail = state->rx_tail;
  uart_critical_leave(primask);
  return head >= tail ? (int)(head - tail)
                      : (int)(UART_RX_CAPACITY + head - tail);
}

int uart_read_device(uart_device_t *device) {
  uart_state_t *state = device->state;
  uint32_t primask;
  uint16_t tail;
  uint8_t byte;

  if (!state->initialized)
    return -1;
  primask = uart_critical_enter();
  if (state->rx_head == state->rx_tail) {
    uart_critical_leave(primask);
    return -1;
  }
  tail = state->rx_tail;
  byte = state->rx_buffer[tail];
  state->rx_tail = (uint16_t)((tail + 1u) % UART_RX_CAPACITY);
  uart_critical_leave(primask);
  return byte;
}

size_t uart_write_device(uart_device_t *device, const void *data,
                         size_t length) {
  const uart_config_t *config = device->config;
  uart_state_t *state = device->state;
  const uint8_t *bytes = data;
  size_t written = 0;
  uint32_t primask;

  if (data == NULL)
    return 0;
  if (!state->initialized)
    return 0;
  primask = uart_critical_enter();
  while (written < length) {
    uint16_t next = (uint16_t)((state->tx_head + 1u) % UART_TX_CAPACITY);
    if (next == state->tx_tail)
      break;
    state->tx_buffer[state->tx_head] = bytes[written++];
    state->tx_head = next;
  }
  if (written != 0)
    config->port->CTRL |= LPUART_CTRL_TIE;
  uart_critical_leave(primask);
  return written;
}

int uart_write_byte_device(uart_device_t *device, uint8_t byte) {
  return uart_write_device(device, &byte, 1) == 1 ? 0 : -1;
}

void uart_flush_device(uart_device_t *device) {
  const uart_config_t *config = device->config;
  uart_state_t *state = device->state;

  if (!state->initialized)
    return;
  while (!uart_tx_empty(state))
    __asm volatile("wfi");
  while (!(config->port->STAT & LPUART_STAT_TC))
    __asm volatile("wfi");
}
