/* Real-time clock + internal temperature.
 *
 * rtc_init() starts the LP SRTC if the counter is not already running
 * (VBAT-backed storage keeps a set time across re-flashes); the demo seeds
 * a known UTC time once and then prints civil date/time each second with
 * the die temperature next to it from the TEMPMON module.
 */

#include <teensy/gpio.h>
#include <teensy/printf.h>
#include <teensy/rtc.h>
#include <teensy/tempmon.h>
#include <teensy/time.h>
#include <teensy/usb.h>

static const char *rtc_weekday_name(uint8_t weekday) {
  static const char *names[7] = {"Sun", "Mon", "Tue", "Wed",
                                 "Thu", "Fri", "Sat"};
  return names[(weekday - 1u) % 7u];
}

int main(void) {
  rtc_datetime_t dt;
  uint32_t milliseconds;

  gpio_configure(13, GPIO_OUTPUT);
  rtc_init();
  if (tempmon_init() != TEMPMON_OK) {
    puts("tempmon init failed");
    return 1;
  }

  /* Remove once a real time has been set somewhere. */
  {
    rtc_datetime_t t = {
        .second = 0, .minute = 24, .hour = 22, .day = 21, .month = 9,
        .year = 2026,
    };
    t.weekday = 1;
    (void)rtc_set_datetime(&t);
  }
  usb_init();

  while (1) {
    gpio_toggle(13);
    (void)rtc_get_ms(&milliseconds);
    if (rtc_get_datetime(&dt) == RTC_OK)
      printf("%s %04u-%02u-%02u %02u:%02u:%02u.%03lu  %.1f C\r\n",
             rtc_weekday_name(dt.weekday), dt.year, dt.month, dt.day,
             dt.hour, dt.minute, dt.second, milliseconds,
             tempmon_get_temp_c());
    else
      puts("rtc not valid");
    time_delay_ms(1000);
  }
}
