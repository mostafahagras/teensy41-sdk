/* GPIO feature tour: typed API, bank mask writes, drive strength, and
 * interrupts.
 *
 * Demonstrated (no wiring needed except one jumper for the interrupt):
 *  1. gpio_configure + driving pins + value-returning gpio_read.
 *  2. Bank mask operations: GPIO_PORT_9 holds Teensy pins 2, 3, 4, 5 at
 *     bits 4..7 and pin 33 at bit 8, so the mask 0x1F0 covers five
 *     chase pins with single atomic writes.
 *  3. gpio_set_drive_strength on every chase pin (weaker than the
 *     default strongest driver: gentler edges for long wires).
 *  4. gpio_attach_interrupt on pin 0 falling edge with the pin's pad
 *     configuration preserved - the handler toggles the LED on every
 *     ground pulse.
 *
 * Wire each of pins 2, 3, 4, 5, 33 through a resistor to an LED to see
 * the chase; pin 13 (onboard LED) blinks on every pin-0 pulse.
 *
 * usb_init() enumerates the CDC port so the loader can soft-reboot this
 * sketch for hands-off uploads.
 */

#include <teensy/gpio.h>
#include <teensy/printf.h>
#include <teensy/time.h>
#include <teensy/usb.h>

static uint32_t pin0_falls;

static void on_pin0_fall(void *_) {
  (void)_;
  pin0_falls++;
  gpio_toggle(13);
}

int main(void) {
  usb_init();
  time_init();

  gpio_configure(13, GPIO_OUTPUT);

  /* Chase pins 2, 3, 4, 5 and 33 all live in GPIO_PORT_9 (bits 4-8):
   * configure them once, then each step is one atomic register write. */
  const uint32_t chase_mask = 0x1F0u;
  for (int i = 4; i <= 8; ++i) {
    uint8_t pin = i == 8 ? 33 : (uint8_t)(i - 2);
    gpio_configure(pin, GPIO_OUTPUT);
    gpio_set_drive_strength(pin, 3); /* medium drive, narrower edges */
  }

  gpio_attach_interrupt(0, GPIO_INTERRUPT_FALLING, on_pin0_fall, (void *)0);

  printf("gpio tour: chase on pins 2/3/4/5/33 (GPIO_PORT_9), medium "
         "drive strength, interrupt on pin 0 (pulse to GND to see it)\r\n");

  uint32_t cursor = 0x10u; /* walking bit, starts at pin 2 (bit 4) */
  while (1) {
    gpio_set_mask(GPIO_PORT_9, cursor);
    gpio_clear_mask(GPIO_PORT_9, chase_mask & ~cursor);
    time_delay_ms(120);
    cursor <<= 1;              /* rotate inside the 5-bit window */
    if (cursor > chase_mask)
      cursor = 0x10u;
  }
}
