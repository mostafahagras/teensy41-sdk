#include <stdarg.h>
#include <stdint.h>

#include <teensy/printf.h>
#include <teensy/usb.h>

static void sdk_printf_putchar(char character, void *argument) {
  (void)argument;
  if (usb_connected())
    (void)usb_write_byte((uint8_t)character);
}

int vprintf(const char *format, va_list arguments) {
  return vfctprintf(sdk_printf_putchar, NULL, format, arguments);
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

  while (*string != '\0') {
    sdk_printf_putchar(*string++, NULL);
    ++result;
  }
  sdk_printf_putchar('\n', NULL);
  return result + 1;
}
