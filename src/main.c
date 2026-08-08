#include <teensy/clock.h>
#include <teensy/gpio.h>
#include <teensy/i2c.h>
#include <teensy/printf.h>
#include <teensy/pwm.h>
#include <teensy/time.h>
#include <teensy/uart.h>
#include <teensy/usb.h>

int main(void) {
  gpio_init();
  pwm_init();
  pwm_set_frequency(13, 1000.0f);
  pwm_write(13, 128);
  uart_init(uart6, 115200);
  i2c_init(i2c1, 100000);
  usb_init();
  printf("teensy41 ready: cpu=%lu bus=%lu\r\n",
         (unsigned long)clock_cpu_frequency_hz,
         (unsigned long)clock_bus_frequency_hz);

  for (;;) {
    __asm volatile("wfi");
  }
}
