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

/* Device-level entry points: operate on an explicit UART device.  Not
 * usually called directly; the public API below resolves to these. */
int uart_init_device(uart_device_t *device, uint32_t baud_rate);
int uart_available_device(uart_device_t *device);
int uart_read_device(uart_device_t *device);
size_t uart_write_device(uart_device_t *device, const void *data,
                         size_t length);
int uart_write_byte_device(uart_device_t *device, uint8_t byte);
void uart_flush_device(uart_device_t *device);

/* Constant-id paths: at a constant id switch(uart) resolves to a single
 * device at compile time; with a runtime id it becomes the dispatch
 * tree.  One macro generates all six so they cannot drift apart. */
#define TEENSY_UART_DEVICE_SWITCH(fn, uart, default_return, ...)               \
  switch (uart) {                                                              \
  case UART_ID_1:                                                              \
    return fn(&uart_device1, ##__VA_ARGS__);                                   \
  case UART_ID_2:                                                              \
    return fn(&uart_device2, ##__VA_ARGS__);                                   \
  case UART_ID_3:                                                              \
    return fn(&uart_device3, ##__VA_ARGS__);                                   \
  case UART_ID_4:                                                              \
    return fn(&uart_device4, ##__VA_ARGS__);                                   \
  case UART_ID_5:                                                              \
    return fn(&uart_device5, ##__VA_ARGS__);                                   \
  case UART_ID_6:                                                              \
    return fn(&uart_device6, ##__VA_ARGS__);                                   \
  case UART_ID_7:                                                              \
    return fn(&uart_device7, ##__VA_ARGS__);                                   \
  case UART_ID_8:                                                              \
    return fn(&uart_device8, ##__VA_ARGS__);                                   \
  default:                                                                     \
    return default_return;                                                     \
  }

static inline
    __attribute__((always_inline)) int uart_init_const(uart_id_t uart,
                                                       uint32_t baud_rate) {
  TEENSY_UART_DEVICE_SWITCH(uart_init_device, uart, -1, baud_rate);
}

static inline __attribute__((always_inline)) int
uart_available_const(uart_id_t uart) {
  TEENSY_UART_DEVICE_SWITCH(uart_available_device, uart, -1);
}

static inline __attribute__((always_inline)) int
uart_read_const(uart_id_t uart) {
  TEENSY_UART_DEVICE_SWITCH(uart_read_device, uart, -1);
}

static inline __attribute__((always_inline)) size_t
uart_write_const(uart_id_t uart, const void *data, size_t length) {
  TEENSY_UART_DEVICE_SWITCH(uart_write_device, uart, 0, data, length);
}

static inline __attribute__((always_inline)) int
uart_write_byte_const(uart_id_t uart, uint8_t byte) {
  TEENSY_UART_DEVICE_SWITCH(uart_write_byte_device, uart, -1, byte);
}

static inline __attribute__((always_inline)) void
uart_flush_const(uart_id_t uart) {
  TEENSY_UART_DEVICE_SWITCH(uart_flush_device, uart, );
}

/* Compile-time UART id validation. */
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

/* Implementation entry points; not usually called directly. */
void uart_attach_rx_impl(uart_id_t uart,
                         void (*callback)(uint8_t byte, void *context),
                         void *context);
void uart_attach_rx_idle_impl(uart_id_t uart, void (*callback)(void *context),
                              void *context);

/** Attaches a handler called from the RX interrupt for every received
 * byte, interrupt context (do not block).  Pass NULL to detach.
 * @param uart One of the uart1..uart8 constants.
 * @param callback Handler, receives the byte and @p context; NULL disables.
 * @param context Passed through to @p callback.
 */
#if defined(__clang__)
static inline void uart_attach_rx(uart_id_t uart,
                                  void (*callback)(uint8_t byte, void *context),
                                  void *context)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline __attribute__((always_inline)) void
uart_attach_rx(uart_id_t uart, void (*callback)(uint8_t byte, void *context),
               void *context) {
  TEENSY_UART_VALIDATE_CONSTANT(uart);
  if (uart < UART_ID_1 || uart > UART_ID_8)
    return;
  uart_attach_rx_impl(uart, callback, context);
}

/** Attaches a handler called when the RX line has been idle for one
 * frame time (end of a burst), interrupt context (do not block).  Useful
 * to collect a whole packet after uart_attach_rx(); only enabled while a
 * handler is set.  Pass NULL to detach.
 * @param uart One of the uart1..uart8 constants.
 * @param callback Handler, receives @p context; NULL disables.
 * @param context Passed through to @p callback.
 */
#if defined(__clang__)
static inline void uart_attach_rx_idle(uart_id_t uart,
                                       void (*callback)(void *context),
                                       void *context)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline __attribute__((always_inline)) void
uart_attach_rx_idle(uart_id_t uart, void (*callback)(void *context),
                    void *context) {
  TEENSY_UART_VALIDATE_CONSTANT(uart);
  if (uart < UART_ID_1 || uart > UART_ID_8)
    return;
  uart_attach_rx_idle_impl(uart, callback, context);
}

/* The public API dispatches on the UART id.  For constant ids the whole
 * chain folds to the polled device's registers at compile time.
 *
 * The const-id validation diagnostic is attached to the public functions
 * themselves (clang's diagnose_if) so language servers flag misuse at the
 * call site; with GCC the in-body __builtin_constant_p check + error
 * attribute covers the same case at compile time. */
#if defined(__clang__)
static inline int uart_init(uart_id_t uart, uint32_t baud_rate) __attribute__((
    diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                "invalid Teensy UART; expected uart1 through uart8", "error")));
#endif
/** Initializes a UART with 8 data bits, no parity, and one stop bit.
 * @param uart One of the uart1..uart8 constants.
 * @param baud_rate Baud rate in bits per second (e.g. 115200).
 * @return 0 on success, or -1 if the UART or baud rate is invalid.
 */
static inline __attribute__((always_inline)) int uart_init(uart_id_t uart,
                                                           uint32_t baud_rate) {
  TEENSY_UART_VALIDATE_CONSTANT(uart);
  if (uart < UART_ID_1 || uart > UART_ID_8)
    return -1;
  return uart_init_const(uart, baud_rate);
}

/** Returns the number of received bytes currently buffered, or -1 for an
 * invalid UART.
 * @param uart One of the uart1..uart8 constants.
 */
static inline __attribute__((always_inline)) int
uart_available(uart_id_t uart) {
  TEENSY_UART_VALIDATE_CONSTANT(uart);
  if (uart < UART_ID_1 || uart > UART_ID_8)
    return -1;
  return uart_available_const(uart);
}

/** Reads one buffered byte.
 * @param uart One of the uart1..uart8 constants.
 * @return The byte as an unsigned value, or -1 if no byte is available or the
 * UART is invalid.
 */
static inline __attribute__((always_inline)) int uart_read(uart_id_t uart) {
  TEENSY_UART_VALIDATE_CONSTANT(uart);
  if (uart < UART_ID_1 || uart > UART_ID_8)
    return -1;
  return uart_read_const(uart);
}

/** Queues bytes for transmission until the transmit buffer is full.
 * @param uart One of the uart1..uart8 constants.
 * @param data Pointer to the bytes to queue.
 * @param length Number of bytes to queue.
 * @return The number of bytes queued, which may be less than @p length, or 0
 * if the UART or buffer is invalid.
 */
static inline __attribute__((always_inline)) size_t uart_write(uart_id_t uart,
                                                               const void *data,
                                                               size_t length) {
  TEENSY_UART_VALIDATE_CONSTANT(uart);
  if (uart < UART_ID_1 || uart > UART_ID_8)
    return 0;
  return uart_write_const(uart, data, length);
}

/** Queues one byte for transmission.
 * @param uart One of the uart1..uart8 constants.
 * @param byte Byte to queue.
 * @return 0 on success, or -1 if the UART is invalid or its buffer is full.
 */
static inline __attribute__((always_inline)) int uart_write_byte(uart_id_t uart,
                                                                 uint8_t byte) {
  TEENSY_UART_VALIDATE_CONSTANT(uart);
  if (uart < UART_ID_1 || uart > UART_ID_8)
    return -1;
  return uart_write_byte_const(uart, byte);
}

/** Blocks until all buffered bytes have been transmitted.
 * @param uart One of the uart1..uart8 constants.
 */
static inline __attribute__((always_inline)) void uart_flush(uart_id_t uart) {
  TEENSY_UART_VALIDATE_CONSTANT(uart);
  if (uart < UART_ID_1 || uart > UART_ID_8)
    return;
  uart_flush_const(uart);
}
#endif
