#ifndef TEENSY_PRINTF_H
#define TEENSY_PRINTF_H

#include <stdarg.h>
#include <stddef.h>

int printf(const char *format, ...);
int vprintf(const char *format, va_list arguments);
int sprintf(char *buffer, const char *format, ...);
int snprintf(char *buffer, size_t count, const char *format, ...);
int vsnprintf(char *buffer, size_t count, const char *format, va_list arguments);
int vfctprintf(void (*output)(char character, void *argument),
               void *argument, const char *format, va_list arguments);

#endif
