/* PWM tour: duty sweep, frequency change, resolution.
 *
 * pwm_init() starts all four FlexPWM controllers and the quad timers
 * with default clocks; pwm_write() addresses a pin through the pwm pin
 * map and pwm_set_frequency() reprograms the owning hardware timer.
 *
 * Wire an LED (+ resistor) from Teensy pin 2 (FlexPWM4 submodule 2,
 * output A on pin 2's mux slot) to GND and watch it breathe; the USB
 * CDC prints the configuration as it changes.  Pin 2's timer is shared
 * with other members of its group, so pwm_set_frequency() on pin 2
 * changes the group-wide period.
 *
 * usb_init() enumerates the CDC port so the loader can soft-reboot this
 * sketch for hands-off uploads.
 */

#include <teensy/printf.h>
#include <teensy/pwm.h>
#include <teensy/time.h>
#include <teensy/usb.h>

int main(void) {
  usb_init();

  pwm_init();
  pwm_set_resolution(12);          /* 0..4095 duty range */
  pwm_set_frequency(2, 1000.0f);   /* 1 kHz carrier on pin 2's group */

  printf("pwm tour: 12-bit 1 kHz breathing LED on pin 2\r\n");

  while (1) {
    /* linear duty ramp 0 -> max -> 0 (triangle wave), 12-bit */
    static int dir = 1;
    static uint32_t duty = 0;

    pwm_write(2, duty);
    time_delay_ms(5);
    if (dir) {
      duty += 8;
      if (duty >= 4095)
        dir = 0;
    } else {
      if (duty > 8)
        duty -= 8;
      else
        dir = 1;
    }
  }
}
