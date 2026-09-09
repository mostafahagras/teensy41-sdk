#ifndef TEENSY_PRINTF_H
#define TEENSY_PRINTF_H

#include <stdarg.h>
#include <stddef.h>

/** Formats output and writes it through the SDK's printf backend.
 * @return The number of characters written.
 */
int printf(const char *format, ...);

/** va_list variant of printf().
 * @return The number of characters written.
 */
int vprintf(const char *format, va_list arguments);

/** Formats output into a buffer without a size limit.
 *
 * The caller must provide enough space for the output and terminating null.
 *
 * @return The number of characters written, excluding the terminating null.
 */
int sprintf(char *buffer, const char *format, ...);

/** Formats output into a size-limited buffer.
 *
 * If @p count is nonzero, the output is null-terminated. The return value can
 * be used to determine the required buffer size when truncation occurs.
 *
 * @return The number of characters that would have been written, excluding
 * the terminating null.
 */
int snprintf(char *buffer, size_t count, const char *format, ...);

/** va_list variant of snprintf().
 * @return The number of characters that would have been written, excluding
 * the terminating null.
 */
int vsnprintf(char *buffer, size_t count, const char *format,
              va_list arguments);

/** Formats output one character at a time through a caller-provided callback.
 *
 * @p argument is passed unchanged to @p output for every emitted character.
 *
 * @return The number of characters emitted.
 */
int vfctprintf(void (*output)(char character, void *argument), void *argument,
               const char *format, va_list arguments);

#endif
