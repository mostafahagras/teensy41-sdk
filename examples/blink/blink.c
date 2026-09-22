/* GPIO blink: the "hello world" of the SDK.
 *
 * Pin 13 on the Teensy 4.1 is the onboard LED.  gpio_configure() sets the
 * pad to output and gpio_toggle() flips the output register; the 1 s cadence
 * comes from the time module's SysTick-backed delay.
 *
 * usb_init() is called even though nothing prints: it enumerates the CDC
 * port so the loader can soft-reboot this sketch for hands-off uploads.
 */

#include <teensy/gpio.h>
#include <teensy/time.h>
#include <teensy/usb.h>

int main(void) {
  usb_init();
  gpio_configure(13, GPIO_OUTPUT);

  while (1) {
    gpio_toggle(13);
    time_delay_ms(1000);
  }
}
