#ifndef TEENSY_UART_H
#define TEENSY_UART_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
  UART_ID_INVALID = 0,
  UART_ID_1 = 1, /* LPUART1 */
  UART_ID_2,     /* LPUART2 */
  UART_ID_3,     /* LPUART3 */
  UART_ID_4,     /* LPUART4 */
  UART_ID_5,     /* LPUART5 */
  UART_ID_6,     /* LPUART6 */
  UART_ID_7,     /* LPUART7 */
  UART_ID_8,     /* LPUART8 */
  UART_COUNT = 9
} uart_id_t;

typedef struct uart_device uart_device_t;

#define uart1 ((uart_id_t)UART_ID_1)
#define uart2 ((uart_id_t)UART_ID_2)
#define uart3 ((uart_id_t)UART_ID_3)
#define uart4 ((uart_id_t)UART_ID_4)
#define uart5 ((uart_id_t)UART_ID_5)
#define uart6 ((uart_id_t)UART_ID_6)
#define uart7 ((uart_id_t)UART_ID_7)
#define uart8 ((uart_id_t)UART_ID_8)

extern uart_device_t uart_device1;
extern uart_device_t uart_device2;
extern uart_device_t uart_device3;
extern uart_device_t uart_device4;
extern uart_device_t uart_device5;
extern uart_device_t uart_device6;
extern uart_device_t uart_device7;
extern uart_device_t uart_device8;

int uart_init_device(uart_device_t *device, uint32_t baud_rate);
int uart_available_device(uart_device_t *device);
int uart_read_device(uart_device_t *device);
size_t uart_write_device(uart_device_t *device, const void *data,
                         size_t length);
int uart_write_byte_device(uart_device_t *device, uint8_t byte);
void uart_flush_device(uart_device_t *device);

/** Initializes a UART with 8 data bits, no parity, and one stop bit.
 * @return 0 on success, or -1 if the UART or baud rate is invalid.
 */
int uart_init(uart_id_t uart, uint32_t baud_rate);

/** Returns the number of received bytes currently buffered, or -1 for an
 * invalid UART.
 */
int uart_available(uart_id_t uart);

/** Reads one buffered byte.
 * @return The byte as an unsigned value, or -1 if no byte is available or the
 * UART is invalid.
 */
int uart_read(uart_id_t uart);

/** Queues bytes for transmission until the transmit buffer is full.
 * @return The number of bytes queued, which may be less than @p length, or 0
 * if the UART or buffer is invalid.
 */
size_t uart_write(uart_id_t uart, const void *data, size_t length);

/** Queues one byte for transmission.
 * @return 0 on success, or -1 if the UART is invalid or its buffer is full.
 */
int uart_write_byte(uart_id_t uart, uint8_t byte);

/** Blocks until all buffered bytes have been transmitted. */
void uart_flush(uart_id_t uart);

static inline __attribute__((always_inline)) int
uart_init_const(uart_id_t uart, uint32_t baud_rate) {
  switch (uart) {
  case UART_ID_1:
    return uart_init_device(&uart_device1, baud_rate);
  case UART_ID_2:
    return uart_init_device(&uart_device2, baud_rate);
  case UART_ID_3:
    return uart_init_device(&uart_device3, baud_rate);
  case UART_ID_4:
    return uart_init_device(&uart_device4, baud_rate);
  case UART_ID_5:
    return uart_init_device(&uart_device5, baud_rate);
  case UART_ID_6:
    return uart_init_device(&uart_device6, baud_rate);
  case UART_ID_7:
    return uart_init_device(&uart_device7, baud_rate);
  case UART_ID_8:
    return uart_init_device(&uart_device8, baud_rate);
  default:
    return -1;
  }
}

static inline __attribute__((always_inline)) int
uart_available_const(uart_id_t uart) {
  switch (uart) {
  case UART_ID_1:
    return uart_available_device(&uart_device1);
  case UART_ID_2:
    return uart_available_device(&uart_device2);
  case UART_ID_3:
    return uart_available_device(&uart_device3);
  case UART_ID_4:
    return uart_available_device(&uart_device4);
  case UART_ID_5:
    return uart_available_device(&uart_device5);
  case UART_ID_6:
    return uart_available_device(&uart_device6);
  case UART_ID_7:
    return uart_available_device(&uart_device7);
  case UART_ID_8:
    return uart_available_device(&uart_device8);
  default:
    return -1;
  }
}

static inline __attribute__((always_inline)) int
uart_read_const(uart_id_t uart) {
  switch (uart) {
  case UART_ID_1:
    return uart_read_device(&uart_device1);
  case UART_ID_2:
    return uart_read_device(&uart_device2);
  case UART_ID_3:
    return uart_read_device(&uart_device3);
  case UART_ID_4:
    return uart_read_device(&uart_device4);
  case UART_ID_5:
    return uart_read_device(&uart_device5);
  case UART_ID_6:
    return uart_read_device(&uart_device6);
  case UART_ID_7:
    return uart_read_device(&uart_device7);
  case UART_ID_8:
    return uart_read_device(&uart_device8);
  default:
    return -1;
  }
}

static inline __attribute__((always_inline)) size_t
uart_write_const(uart_id_t uart, const void *data, size_t length) {
  switch (uart) {
  case UART_ID_1:
    return uart_write_device(&uart_device1, data, length);
  case UART_ID_2:
    return uart_write_device(&uart_device2, data, length);
  case UART_ID_3:
    return uart_write_device(&uart_device3, data, length);
  case UART_ID_4:
    return uart_write_device(&uart_device4, data, length);
  case UART_ID_5:
    return uart_write_device(&uart_device5, data, length);
  case UART_ID_6:
    return uart_write_device(&uart_device6, data, length);
  case UART_ID_7:
    return uart_write_device(&uart_device7, data, length);
  case UART_ID_8:
    return uart_write_device(&uart_device8, data, length);
  default:
    return 0;
  }
}

static inline __attribute__((always_inline)) int
uart_write_byte_const(uart_id_t uart, uint8_t byte) {
  switch (uart) {
  case UART_ID_1:
    return uart_write_byte_device(&uart_device1, byte);
  case UART_ID_2:
    return uart_write_byte_device(&uart_device2, byte);
  case UART_ID_3:
    return uart_write_byte_device(&uart_device3, byte);
  case UART_ID_4:
    return uart_write_byte_device(&uart_device4, byte);
  case UART_ID_5:
    return uart_write_byte_device(&uart_device5, byte);
  case UART_ID_6:
    return uart_write_byte_device(&uart_device6, byte);
  case UART_ID_7:
    return uart_write_byte_device(&uart_device7, byte);
  case UART_ID_8:
    return uart_write_byte_device(&uart_device8, byte);
  default:
    return -1;
  }
}

static inline __attribute__((always_inline)) void
uart_flush_const(uart_id_t uart) {
  switch (uart) {
  case UART_ID_1:
    uart_flush_device(&uart_device1);
    break;
  case UART_ID_2:
    uart_flush_device(&uart_device2);
    break;
  case UART_ID_3:
    uart_flush_device(&uart_device3);
    break;
  case UART_ID_4:
    uart_flush_device(&uart_device4);
    break;
  case UART_ID_5:
    uart_flush_device(&uart_device5);
    break;
  case UART_ID_6:
    uart_flush_device(&uart_device6);
    break;
  case UART_ID_7:
    uart_flush_device(&uart_device7);
    break;
  case UART_ID_8:
    uart_flush_device(&uart_device8);
    break;
  default:
    break;
  }
}

#ifndef TEENSY_UART_IMPLEMENTATION
#if defined(__clang__)
static inline void uart_validate(uart_id_t uart) __attribute__((
    diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                "invalid Teensy UART; expected uart1 through uart8", "error")));
static inline void uart_validate(uart_id_t uart) { (void)uart; }
#else
extern void uart_invalid_constant(void)
    __attribute__((error("invalid Teensy UART; expected uart1 through uart8")));
#endif

#if defined(__clang__)
#define TEENSY_UART_VALIDATE_CONSTANT(uart) uart_validate((uart_id_t)(uart))
#else
#define TEENSY_UART_VALIDATE_CONSTANT(uart)                                    \
  ({                                                                           \
    if (__builtin_constant_p(uart) &&                                          \
        !((uart) >= UART_ID_1 && (uart) <= UART_ID_8))                         \
      uart_invalid_constant();                                                 \
    (void)0;                                                                   \
  })
#endif

#define uart_init(uart, baud_rate)                                             \
  ({                                                                           \
    TEENSY_UART_VALIDATE_CONSTANT(uart);                                       \
    __builtin_choose_expr(__builtin_constant_p(uart),                          \
                          uart_init_const((uart_id_t)(uart), (baud_rate)),     \
                          uart_init((uart_id_t)(uart), (baud_rate)));          \
  })

#define uart_available(uart)                                                   \
  ({                                                                           \
    TEENSY_UART_VALIDATE_CONSTANT(uart);                                       \
    __builtin_choose_expr(__builtin_constant_p(uart),                          \
                          uart_available_const((uart_id_t)(uart)),             \
                          uart_available((uart_id_t)(uart)));                  \
  })

#define uart_read(uart)                                                        \
  ({                                                                           \
    TEENSY_UART_VALIDATE_CONSTANT(uart);                                       \
    __builtin_choose_expr(__builtin_constant_p(uart),                          \
                          uart_read_const((uart_id_t)(uart)),                  \
                          uart_read((uart_id_t)(uart)));                       \
  })

#define uart_write(uart, data, length)                                         \
  ({                                                                           \
    TEENSY_UART_VALIDATE_CONSTANT(uart);                                       \
    __builtin_choose_expr(                                                     \
        __builtin_constant_p(uart),                                            \
        uart_write_const((uart_id_t)(uart), (data), (length)),                 \
        uart_write((uart_id_t)(uart), (data), (length)));                      \
  })

#define uart_write_byte(uart, byte)                                            \
  ({                                                                           \
    TEENSY_UART_VALIDATE_CONSTANT(uart);                                       \
    __builtin_choose_expr(__builtin_constant_p(uart),                          \
                          uart_write_byte_const((uart_id_t)(uart), (byte)),    \
                          uart_write_byte((uart_id_t)(uart), (byte)));         \
  })

#define uart_flush(uart)                                                       \
  ({                                                                           \
    TEENSY_UART_VALIDATE_CONSTANT(uart);                                       \
    __builtin_choose_expr(__builtin_constant_p(uart),                          \
                          uart_flush_const((uart_id_t)(uart)),                 \
                          uart_flush((uart_id_t)(uart)));                      \
  })
#endif

#endif
