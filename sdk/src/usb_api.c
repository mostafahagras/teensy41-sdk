#include <stddef.h>
#include <stdint.h>

#include <teensy/imxrt.h>
#include <teensy/time.h>
#include <teensy/usb.h>
#include <teensy/usb_dev.h>
#include <teensy/usb_serial.h>

#define USB_STARTUP_DELAY_MS 20u

extern volatile uint8_t usb_configuration;

static void usb_pll_start(void) {
  for (;;) {
    uint32_t status = CCM_ANALOG_PLL_USB1;

    if (status & CCM_ANALOG_PLL_USB1_DIV_SELECT) {
      CCM_ANALOG_PLL_USB1_CLR = 0xC000u;
      CCM_ANALOG_PLL_USB1_SET = CCM_ANALOG_PLL_USB1_BYPASS;
      CCM_ANALOG_PLL_USB1_CLR =
          CCM_ANALOG_PLL_USB1_POWER | CCM_ANALOG_PLL_USB1_DIV_SELECT |
          CCM_ANALOG_PLL_USB1_ENABLE | CCM_ANALOG_PLL_USB1_EN_USB_CLKS;
      continue;
    }
    if (!(status & CCM_ANALOG_PLL_USB1_ENABLE)) {
      CCM_ANALOG_PLL_USB1_SET = CCM_ANALOG_PLL_USB1_ENABLE;
      continue;
    }
    if (!(status & CCM_ANALOG_PLL_USB1_POWER)) {
      CCM_ANALOG_PLL_USB1_SET = CCM_ANALOG_PLL_USB1_POWER;
      continue;
    }
    if (!(status & CCM_ANALOG_PLL_USB1_LOCK))
      continue;
    if (status & CCM_ANALOG_PLL_USB1_BYPASS) {
      CCM_ANALOG_PLL_USB1_CLR = CCM_ANALOG_PLL_USB1_BYPASS;
      continue;
    }
    if (!(status & CCM_ANALOG_PLL_USB1_EN_USB_CLKS)) {
      CCM_ANALOG_PLL_USB1_SET = CCM_ANALOG_PLL_USB1_EN_USB_CLKS;
      continue;
    }
    return;
  }
}

void usb_init(void) {
  uint32_t elapsed = time_millis();

  /* Teensyduino does not start the controller during the first 20 ms after
   * reset.  Preserve that hardware stabilization interval while keeping USB
   * opt-in and avoiding Arduino's additional 280 ms application delay. */
  if (elapsed < USB_STARTUP_DELAY_MS)
    time_delay_ms(USB_STARTUP_DELAY_MS - elapsed);
  usb_pll_start();
  usb_controller_init();
}

int usb_connected(void) {
  return usb_configuration != 0 && (usb_cdc_line_rtsdtr & USB_SERIAL_DTR);
}

int usb_available(void) { return usb_serial_available(); }

int usb_read(void) { return usb_serial_getchar(); }

size_t usb_write(const void *data, size_t length) {
  return usb_serial_write(data, length);
}

int usb_write_byte(uint8_t byte) { return usb_serial_putchar(byte); }

void usb_flush(void) { usb_serial_flush_output(); }
