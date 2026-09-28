/* UART echo with RX interrupt handlers: the "hello world" of the uart SDK.
 *
 * uart1 drives the Teensy pins labeled RX1/TX1 (header pins 0 and 1).
 * Wiring: peer TX -> Teensy pin 0, peer RX <- Teensy pin 1, GND to GND,
 * 115200 8N1 on the peer.
 *
 * Demonstrates the interrupt-driven receive path (uart_attach_rx +
 * uart_attach_rx_idle), the runtime reporting of the achieved baud rate
 * (uart_set_baud returns what the hardware ends up at), and the ring
 * counters (uart_available / uart_available_for_write).  The byte
 * handler echoes in interrupt context; the idle handler flashes the
 * LED at the end of each incoming burst; a periodic status line goes to
 * the USB CDC port.
 *
 * usb_init() enumerates the CDC port so the loader can soft-reboot this
 * sketch for hands-off uploads.
 */

#include <teensy/gpio.h>
#include <teensy/printf.h>
#include <teensy/time.h>
#include <teensy/uart.h>
#include <teensy/usb.h>

static uint32_t received_bytes;
static uint32_t received_bursts;

static void on_rx_byte(uint8_t byte, void *_) {
  (void)_;
  received_bytes++;
  uart_write_byte(uart1, byte); /* interrupt context: keep it quick */
}

static void on_rx_idle(void *_) {
  (void)_;
  received_bursts++;
  gpio_toggle(13); /* LED flash marks end of each incoming burst */
}

int main(void) {
  usb_init();

  gpio_configure(13, GPIO_OUTPUT);
  uart_init(uart1, 115200);
  uart_attach_rx(uart1, on_rx_byte, (void *)0);
  uart_attach_rx_idle(uart1, on_rx_idle, (void *)0);
  /* uart_set_baud() reports the achieved rate; calling it again with the
   * same requested value returns the same hardware divisor. */
  uint32_t actual_baud = uart_set_baud(uart1, 115200);

  printf("uart1 %lu 8N1, echoes every byte; bursts flash the LED\r\n",
         (unsigned long)actual_baud);

  while (1) {
    printf("rx=%lu tx_free=%d bursts=%lu\r\n", (unsigned long)received_bytes,
           uart_available_for_write(uart1), (unsigned long)received_bursts);
    time_delay_ms(1000);
  }
}
