#include <teensy/gpio.h>
#include <teensy/time.h>

int main(void) {
  gpio_configure(13, GPIO_OUTPUT);

  while (1) {
    gpio_toggle(13);
    time_delay_ms(1000);
  }
}
