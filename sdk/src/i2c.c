#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <teensy/gpio.h>
#include <teensy/i2c.h>
#include <teensy/imxrt.h>
#include <teensy/time.h>

#define I2C_TIMEOUT_US 15000u
#define I2C_PINCONFIG                                                          \
  (IOMUXC_PAD_ODE | IOMUXC_PAD_SRE | IOMUXC_PAD_DSE(4) | IOMUXC_PAD_SPEED(1) | \
   IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_PUS(3) | IOMUXC_PAD_HYS)

typedef struct {
  IMXRT_LPI2C_t *port;
  volatile uint32_t *clock_gate;
  uint32_t clock_gate_mask;
  uint8_t sda_pin;
  uint8_t scl_pin;
  volatile uint32_t *sda_mux;
  volatile uint32_t *sda_pad;
  volatile uint32_t *sda_select;
  uint32_t sda_mux_value;
  uint32_t sda_select_value;
  volatile uint32_t *scl_mux;
  volatile uint32_t *scl_pad;
  volatile uint32_t *scl_select;
  uint32_t scl_mux_value;
  uint32_t scl_select_value;
} i2c_config_t;

static const i2c_config_t i2c_configs[I2C_COUNT] = {
    [I2C_ID_1] = {&IMXRT_LPI2C1, &CCM_CCGR2, CCM_CCGR2_LPI2C1(CCM_CCGR_ON), 18,
                  19, &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_01,
                  &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_01,
                  &IOMUXC_LPI2C1_SDA_SELECT_INPUT, 3u | 0x10u, 1,
                  &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_00,
                  &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_00,
                  &IOMUXC_LPI2C1_SCL_SELECT_INPUT, 3u | 0x10u, 1},
    [I2C_ID_3] = {&IMXRT_LPI2C3, &CCM_CCGR2, CCM_CCGR2_LPI2C3(CCM_CCGR_ON), 17,
                  16, &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_06,
                  &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_06,
                  &IOMUXC_LPI2C3_SDA_SELECT_INPUT, 1u | 0x10u, 2,
                  &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_07,
                  &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_07,
                  &IOMUXC_LPI2C3_SCL_SELECT_INPUT, 1u | 0x10u, 2},
    [I2C_ID_4] = {
        &IMXRT_LPI2C4, &CCM_CCGR6, CCM_CCGR6_LPI2C4_SERIAL(CCM_CCGR_ON), 25, 24,
        &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_13,
        &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_13, &IOMUXC_LPI2C4_SDA_SELECT_INPUT,
        0x10u, 1, &IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_12,
        &IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_12, &IOMUXC_LPI2C4_SCL_SELECT_INPUT,
        0x10u, 1}};

static bool i2c_valid(i2c_id_t bus) {
  return bus == I2C_ID_1 || bus == I2C_ID_3 || bus == I2C_ID_4;
}

static void i2c_clear_fifos(IMXRT_LPI2C_t *port) {
  port->MCR |= LPI2C_MCR_RTF | LPI2C_MCR_RRF;
}

static void i2c_clear_status(IMXRT_LPI2C_t *port) {
  port->MSR = LPI2C_MSR_DMF | LPI2C_MSR_PLTF | LPI2C_MSR_FEF | LPI2C_MSR_ALF |
              LPI2C_MSR_NDF | LPI2C_MSR_SDF | LPI2C_MSR_EPF;
}

static int i2c_wait_idle(IMXRT_LPI2C_t *port) {
  uint32_t start_time = time_micros();

  while ((port->MSR & LPI2C_MSR_BBF) && !(port->MSR & LPI2C_MSR_MBF)) {
    if ((uint32_t)(time_micros() - start_time) > I2C_TIMEOUT_US) {
      return I2C_ERROR_TIMEOUT;
    }
  }
  return I2C_OK;
}

static int i2c_set_frequency(IMXRT_LPI2C_t *port, uint32_t frequency_hz) {
  port->MCR = 0;
  if (frequency_hz == 0)
    return I2C_ERROR_INVALID;
  if (frequency_hz <= 100000u) {
    port->MCCR0 = LPI2C_MCCR0_CLKHI(55) | LPI2C_MCCR0_CLKLO(59) |
                  LPI2C_MCCR0_DATAVD(25) | LPI2C_MCCR0_SETHOLD(40);
    port->MCFGR1 = LPI2C_MCFGR1_PRESCALE(1);
    port->MCFGR2 = LPI2C_MCFGR2_FILTSDA(5) | LPI2C_MCFGR2_FILTSCL(5) |
                   LPI2C_MCFGR2_BUSIDLE(3000);
    port->MCFGR3 = LPI2C_MCFGR3_PINLOW(704);
  } else if (frequency_hz <= 400000u) {
    port->MCCR0 = LPI2C_MCCR0_CLKHI(26) | LPI2C_MCCR0_CLKLO(28) |
                  LPI2C_MCCR0_DATAVD(12) | LPI2C_MCCR0_SETHOLD(18);
    port->MCFGR1 = LPI2C_MCFGR1_PRESCALE(0);
    port->MCFGR2 = LPI2C_MCFGR2_FILTSDA(2) | LPI2C_MCFGR2_FILTSCL(2) |
                   LPI2C_MCFGR2_BUSIDLE(3600);
    port->MCFGR3 = LPI2C_MCFGR3_PINLOW(1407);
  } else if (frequency_hz <= 1000000u) {
    port->MCCR0 = LPI2C_MCCR0_CLKHI(9) | LPI2C_MCCR0_CLKLO(10) |
                  LPI2C_MCCR0_DATAVD(4) | LPI2C_MCCR0_SETHOLD(7);
    port->MCFGR1 = LPI2C_MCFGR1_PRESCALE(0);
    port->MCFGR2 = LPI2C_MCFGR2_FILTSDA(1) | LPI2C_MCFGR2_FILTSCL(1) |
                   LPI2C_MCFGR2_BUSIDLE(2400);
    port->MCFGR3 = LPI2C_MCFGR3_PINLOW(1407);
  } else {
    return I2C_ERROR_INVALID;
  }
  port->MCCR1 = port->MCCR0;
  port->MCFGR0 = 0;
  port->MFCR = LPI2C_MFCR_RXWATER(1) | LPI2C_MFCR_TXWATER(1);
  port->MCR = LPI2C_MCR_MEN;
  return I2C_OK;
}

int i2c_init(i2c_id_t bus, uint32_t frequency_hz) {
  const i2c_config_t *config;
  IMXRT_LPI2C_t *port;

  if (!i2c_valid(bus))
    return I2C_ERROR_INVALID;
  config = &i2c_configs[bus];
  port = config->port;

  if (gpio_configure(config->sda_pin, GPIO_INPUT) != 0 ||
      gpio_configure(config->scl_pin, GPIO_INPUT) != 0) {
    return I2C_ERROR_INVALID;
  }
  *config->clock_gate |= config->clock_gate_mask;
  *config->sda_pad = I2C_PINCONFIG;
  *config->sda_mux = config->sda_mux_value;
  *config->sda_select = config->sda_select_value;
  *config->scl_pad = I2C_PINCONFIG;
  *config->scl_mux = config->scl_mux_value;
  *config->scl_select = config->scl_select_value;

  port->MCR = LPI2C_MCR_RST;
  port->MCR = 0;
  return i2c_set_frequency(port, frequency_hz);
}

static int i2c_transaction(const i2c_config_t *config, uint8_t address,
                           const uint8_t *write_data, size_t write_length,
                           uint8_t *read_data, size_t read_length) {
  IMXRT_LPI2C_t *port = config->port;
  size_t write_index = 0;
  size_t read_index = 0;
  uint8_t stage = write_length ? 0u : 2u;
  bool receive_command_queued = false;
  bool stop_queued = false;
  uint32_t start_time = time_micros();

  if (write_length > 255 || read_length > 256)
    return I2C_ERROR_INVALID;
  if (i2c_wait_idle(port) != I2C_OK)
    return I2C_ERROR_TIMEOUT;
  i2c_clear_fifos(port);
  i2c_clear_status(port);

  for (;;) {
    uint32_t status;
    uint32_t tx_count;

    tx_count = port->MFSR & 0x07u;
    while (tx_count < 4u && !stop_queued) {
      if (stage == 0) {
        port->MTDR =
            LPI2C_MTDR_CMD_START | LPI2C_MTDR_DATA((address & 0x7Fu) << 1);
        stage = 1;
      } else if (stage == 1 && write_index < write_length) {
        port->MTDR = LPI2C_MTDR_CMD_TRANSMIT |
                     LPI2C_MTDR_DATA(write_data[write_index++]);
        if (write_index == write_length)
          stage = read_length ? 2u : 4u;
      } else if (stage == 2 && read_length != 0) {
        port->MTDR = LPI2C_MTDR_CMD_START |
                     LPI2C_MTDR_DATA(((address & 0x7Fu) << 1) | 1u);
        stage = 3;
      } else if (stage == 3 && !receive_command_queued) {
        port->MTDR = LPI2C_MTDR_CMD_RECEIVE |
                     LPI2C_MTDR_DATA((uint32_t)read_length - 1u);
        receive_command_queued = true;
        stage = 4;
      } else if (stage == 4) {
        port->MTDR = LPI2C_MTDR_CMD_STOP;
        stop_queued = true;
      } else {
        break;
      }
      ++tx_count;
    }

    while (((port->MFSR >> 16) & 0x07u) != 0 && read_index < read_length) {
      read_data[read_index++] = (uint8_t)port->MRDR;
    }

    status = port->MSR;
    if (status & LPI2C_MSR_NDF) {
      i2c_clear_fifos(port);
      return I2C_ERROR_NACK;
    }
    if (status & LPI2C_MSR_ALF) {
      i2c_clear_fifos(port);
      return I2C_ERROR_ARBITRATION;
    }
    if (status & LPI2C_MSR_FEF) {
      i2c_clear_fifos(port);
      return I2C_ERROR_FIFO;
    }
    if (status & LPI2C_MSR_PLTF ||
        (uint32_t)(time_micros() - start_time) > I2C_TIMEOUT_US) {
      i2c_clear_fifos(port);
      port->MTDR = LPI2C_MTDR_CMD_STOP;
      return I2C_ERROR_TIMEOUT;
    }

    if (stop_queued && read_index == read_length && (port->MFSR & 0x07u) == 0 &&
        (status & LPI2C_MSR_SDF)) {
      return I2C_OK;
    }
    __asm volatile("nop");
  }
}

int i2c_write(i2c_id_t bus, uint8_t address, const void *data, size_t length) {
  if (!i2c_valid(bus) || data == NULL || length == 0)
    return I2C_ERROR_INVALID;
  return i2c_transaction(&i2c_configs[bus], address, data, length, NULL, 0);
}

int i2c_read(i2c_id_t bus, uint8_t address, void *data, size_t length) {
  if (!i2c_valid(bus) || data == NULL || length == 0)
    return I2C_ERROR_INVALID;
  return i2c_transaction(&i2c_configs[bus], address, NULL, 0, data, length);
}

int i2c_write_read(i2c_id_t bus, uint8_t address, const void *write_data,
                   size_t write_length, void *read_data, size_t read_length) {
  if (!i2c_valid(bus) || write_data == NULL || read_data == NULL ||
      write_length == 0 || read_length == 0) {
    return I2C_ERROR_INVALID;
  }
  return i2c_transaction(&i2c_configs[bus], address, write_data, write_length,
                         read_data, read_length);
}
