/* Internal die temperature via the TEMPMON peripheral.
 *
 * The sensor is factory calibrated from the OCOTP fuses; measuring is a
 * matter of waiting for the next finished conversion.  The default
 * alarm thresholds (85/25/90 C) match Teensyduino's; the panic alarm
 * halts the chip below them.
 */

#include <teensy/printf.h>
#include <teensy/tempmon.h>
#include <teensy/time.h>
#include <teensy/usb.h>

int main(void) {
  usb_init();

  if (tempmon_init() != TEMPMON_OK) {
    puts("tempmon init failed");
    return 1;
  }

  while (1) {
    printf("cpu temp: %.2f C (%.2f F)\r\n", tempmon_get_temp_c(),
           tempmon_get_temp_f());
    time_delay_ms(1000);
  }
}
