#include <stdarg.h>
#include <stdint.h>

#include <teensy/printf.h>
#include <teensy/usb.h>

static void sdk_printf_putchar(char character, void *argument) {
  (void)argument;
  (void)usb_write_byte((uint8_t)character);
}

static void sdk_printf_discard(char character, void *argument) {
  (void)character;
  (void)argument;
}

int vprintf(const char *format, va_list arguments) {
  if (usb_connected())
    return vfctprintf(sdk_printf_putchar, NULL, format, arguments);
  return vfctprintf(sdk_printf_discard, NULL, format, arguments);
}

int printf(const char *format, ...) {
  va_list arguments;
  int result;

  va_start(arguments, format);
  result = vprintf(format, arguments);
  va_end(arguments);
  return result;
}

int puts(const char *string) {
  int result = 0;

  if (usb_connected()) {
    while (*string != '\0') {
      (void)usb_write_byte((uint8_t)*string++);
      ++result;
    }
    (void)usb_write_byte('\n');
  } else {
    const volatile char *cursor = (const volatile char *)string;
    while (*cursor++ != '\0')
      ++result;
  }

  return result + 1;
}
