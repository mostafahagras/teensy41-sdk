#ifndef TEENSY_UART_H
#define TEENSY_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Uarts are numbered after the Teensy silkscreen RX<N>/TX<N> pin pairs:
 * uart1 drives the pins labeled RX1/TX1, etc.  The underlying hardware
 * LPUART is listed for reference. */
typedef enum {
  UART_ID_INVALID = 0,
  UART_ID_1 = 1, /* LPUART6,  Teensy pins RX1 = 0, TX1 = 1   */
  UART_ID_2,     /* LPUART4,  Teensy pins RX2 = 7, TX2 = 8   */
  UART_ID_3,     /* LPUART2,  Teensy pins TX3 = 14, RX3 = 15 */
  UART_ID_4,     /* LPUART3,  Teensy pins RX4 = 16, TX4 = 17 */
  UART_ID_5,     /* LPUART8,  Teensy pins TX5 = 20, RX5 = 21 */
  UART_ID_6,     /* LPUART1,  Teensy pins TX6 = 24, RX6 = 25 */
  UART_ID_7,     /* LPUART7,  Teensy pins RX7 = 28, TX7 = 29 */
  UART_ID_8,     /* LPUART5,  Teensy pins RX8 = 34, TX8 = 35 */
  UART_COUNT = 9
} uart_id_t;

typedef enum {
  UART_PARITY_NONE = 0,
  UART_PARITY_EVEN = 1,
  UART_PARITY_ODD = 2
} uart_parity_t;

/* Opaque device handle.
 * @internal
 * Normally only reachable through the public API; exposed so the rare
 * advanced use case (registering a device table of your own) can work. */
typedef struct uart_device uart_device_t;

#define uart1 ((uart_id_t)UART_ID_1)
#define uart2 ((uart_id_t)UART_ID_2)
#define uart3 ((uart_id_t)UART_ID_3)
#define uart4 ((uart_id_t)UART_ID_4)
#define uart5 ((uart_id_t)UART_ID_5)
#define uart6 ((uart_id_t)UART_ID_6)
#define uart7 ((uart_id_t)UART_ID_7)
#define uart8 ((uart_id_t)UART_ID_8)

/* ============================ INTERNAL API ============================ */
/* Everything below operates on an explicit uart_device_t and is what
 * the public API resolves to.  Not for application use unless you have
 * a specific reason.
 * @internal */

extern uart_device_t uart_device1;
extern uart_device_t uart_device2;
extern uart_device_t uart_device3;
extern uart_device_t uart_device4;
extern uart_device_t uart_device5;
extern uart_device_t uart_device6;
extern uart_device_t uart_device7;
extern uart_device_t uart_device8;

/* Compile-time UART id validation: as a call-site diagnose_if attribute
 * for language servers (clang), and as an unreachable error-attribute
 * branch folded during optimization for GCC. */
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

/* Constant-id dispatch generator: at a constant id switch(uart) resolves
 * to a single device at compile time; with a runtime id it becomes the
 * dispatch tree. */
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

/* Void-returning variant. */
#define TEENSY_UART_DEVICE_SWITCH_VOID(fn, uart, ...)                          \
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
    return;                                                                    \
  }

int uart_init_device(uart_device_t *device, uint32_t baud_rate);
int uart_available_device(uart_device_t *device);
int uart_read_device(uart_device_t *device);
size_t uart_write_device(uart_device_t *device, const void *data,
                         size_t length);
int uart_write_byte_device(uart_device_t *device, uint8_t byte);
void uart_flush_device(uart_device_t *device);
void uart_clear_device(uart_device_t *device);
int uart_available_for_write_device(uart_device_t *device);
bool uart_is_readable_within_us_device(uart_device_t *device, uint32_t us);
int uart_set_format_device(uart_device_t *device, uint8_t data_bits,
                           uint8_t stop_bits, uart_parity_t parity);
uint32_t uart_set_baud_device(uart_device_t *device, uint32_t baud_rate);
void uart_attach_rx_device(uart_device_t *device,
                           void (*callback)(uint8_t byte, void *context),
                           void *context);
void uart_attach_rx_idle_device(uart_device_t *device,
                                void (*callback)(void *context), void *context);

/* ===================== PUBLIC API IMPLEMENTATIONS ===================== */

/* The public functions dispatch on the UART id.  For constant ids the
 * whole chain folds straight down to the device's registers; the
 * misused-id diagnostic attached above (clang) and the in-body
 * validator (GCC) cover constant misuse in builds too. */
#if defined(__clang__)
#define TEENSY_UART_VALIDATE_ID(uart, invalid_result)                          \
  TEENSY_UART_VALIDATE_CONSTANT(uart);                                         \
  if (uart < UART_ID_1 || uart > UART_ID_8)                                    \
    return invalid_result;
#else
#define TEENSY_UART_VALIDATE_ID(uart, invalid_result)                          \
  TEENSY_UART_VALIDATE_CONSTANT(uart);                                         \
  if (uart < UART_ID_1 || uart > UART_ID_8)                                    \
    return invalid_result;
#endif

/** Initializes a UART with 8 data bits, no parity, and one stop bit.
 * @param uart One of the uart1..uart8 constants.
 * @param baud_rate Baud rate in bits per second (e.g. 115200).
 * @return 0 on success, or -1 if the UART or baud rate is invalid.
 */
#if defined(__clang__)
static inline int uart_init(uart_id_t uart, uint32_t baud_rate)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline __attribute__((always_inline)) int uart_init(uart_id_t uart,
                                                           uint32_t baud_rate) {
  TEENSY_UART_VALIDATE_ID(uart, -1)
  TEENSY_UART_DEVICE_SWITCH(uart_init_device, uart, -1, baud_rate);
}

/** Returns the number of received bytes currently buffered, or -1 for an
 * invalid UART.
 * @param uart One of the uart1..uart8 constants.
 */
#if defined(__clang__)
static inline int uart_read(uart_id_t uart)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline
    __attribute__((always_inline)) int uart_available(uart_id_t uart) {
  TEENSY_UART_VALIDATE_ID(uart, -1)
  TEENSY_UART_DEVICE_SWITCH(uart_available_device, uart, -1);
}

/** Reads one buffered byte.
 * @param uart One of the uart1..uart8 constants.
 * @return The byte as an unsigned value, or -1 if no byte is available or the
 * UART is invalid.
 */
#if defined(__clang__)
static inline int uart_read(uart_id_t uart)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline __attribute__((always_inline)) int uart_read(uart_id_t uart) {
  TEENSY_UART_VALIDATE_ID(uart, -1)
  TEENSY_UART_DEVICE_SWITCH(uart_read_device, uart, -1);
}

/** Queues bytes for transmission until the transmit buffer is full.
 * @param uart One of the uart1..uart8 constants.
 * @param data Pointer to the bytes to queue.
 * @param length Number of bytes to queue.
 * @return The number of bytes queued, which may be less than @p length, or 0
 * if the UART or buffer is invalid.
 */
#if defined(__clang__)
static inline int uart_write_byte(uart_id_t uart, uint8_t byte)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline __attribute__((always_inline))
size_t uart_write(uart_id_t uart, const void *data, size_t length) {
  TEENSY_UART_VALIDATE_ID(uart, 0)
  TEENSY_UART_DEVICE_SWITCH(uart_write_device, uart, 0, data, length);
}

/** Queues one byte for transmission.
 * @param uart One of the uart1..uart8 constants.
 * @param byte Byte to queue.
 * @return 0 on success, or -1 if the UART is invalid or its buffer is full.
 */
#if defined(__clang__)
static inline int uart_write_byte(uart_id_t uart, uint8_t byte)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline __attribute__((always_inline)) int uart_write_byte(uart_id_t uart,
                                                                 uint8_t byte) {
  TEENSY_UART_VALIDATE_ID(uart, -1)
  TEENSY_UART_DEVICE_SWITCH(uart_write_byte_device, uart, -1, byte);
}

/** Blocks until all buffered bytes have been transmitted.
 * @param uart One of the uart1..uart8 constants.
 */
#if defined(__clang__)
static inline void uart_flush(uart_id_t uart)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline __attribute__((always_inline)) void uart_flush(uart_id_t uart) {
  TEENSY_UART_VALIDATE_CONSTANT(uart);
  if (uart < UART_ID_1 || uart > UART_ID_8)
    return;
  TEENSY_UART_DEVICE_SWITCH_VOID(uart_flush_device, uart);
}

/** Sets the data/stop/parity framing of an initialized UART.  LPUART
 * hardware constraint: 7 data bits require a parity bit; 5/6-bit frames
 * are not expressible.  Pauses transmission until the current frame
 * completes.
 * @param uart One of the uart1..uart8 constants.
 * @param data_bits 7 or 8 (7 requires parity).
 * @param stop_bits 1 or 2.
 * @param parity UART_PARITY_NONE, UART_PARITY_EVEN or UART_PARITY_ODD.
 * @return 0 on success, -1 if the UART is uninitialized or the framing
 * is not expressible in LPUART hardware.
 */
#if defined(__clang__)
static inline void uart_clear(uart_id_t uart)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline __attribute__((always_inline)) int
uart_set_format(uart_id_t uart, uint8_t data_bits, uint8_t stop_bits,
                uart_parity_t parity) {
  TEENSY_UART_VALIDATE_ID(uart, -1)
  TEENSY_UART_DEVICE_SWITCH(uart_set_format_device, uart, -1, data_bits,
                            stop_bits, parity);
}

/** Changes the baud rate of an initialized UART.
 * @param uart One of the uart1..uart8 constants.
 * @param baud_rate Requested baud in bits per second.
 * @return The achieved baud (closest hardware divisor), or 0 on error.
 */
#if defined(__clang__)
static inline void uart_clear(uart_id_t uart)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline
    __attribute__((always_inline)) uint32_t uart_set_baud(uart_id_t uart,
                                                          uint32_t baud_rate) {
  TEENSY_UART_VALIDATE_ID(uart, 0)
  TEENSY_UART_DEVICE_SWITCH(uart_set_baud_device, uart, 0, baud_rate);
}

/** Attaches a handler called from the RX interrupt for every received
 * byte, interrupt context (do not block).  Pass NULL to detach.
 * @param uart One of the uart1..uart8 constants.
 * @param callback Handler, receives the byte and @p context; NULL disables.
 * @param context Passed through to @p callback.
 */
#if defined(__clang__)
static inline void uart_clear(uart_id_t uart)
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
  TEENSY_UART_DEVICE_SWITCH_VOID(uart_attach_rx_device, uart, callback,
                                 context);
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
static inline void uart_clear(uart_id_t uart)
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
  TEENSY_UART_DEVICE_SWITCH_VOID(uart_attach_rx_idle_device, uart, callback,
                                 context);
}

/** Discards all received-but-unread bytes of a UART.
 * @param uart One of the uart1..uart8 constants.
 */
#if defined(__clang__)
static inline void uart_clear(uart_id_t uart)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline __attribute__((always_inline)) void uart_clear(uart_id_t uart) {
  TEENSY_UART_VALIDATE_CONSTANT(uart);
  if (uart < UART_ID_1 || uart > UART_ID_8)
    return;
  TEENSY_UART_DEVICE_SWITCH_VOID(uart_clear_device, uart);
}

/** Returns the number of bytes that uart_write() would queue into the
 * transmit buffer without blocking.
 * @param uart One of the uart1..uart8 constants.
 * @return Free bytes in the transmit ring, or 0 if uninitialized.
 */
#if defined(__clang__)
static inline int uart_available_for_write(uart_id_t uart)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline __attribute__((always_inline)) int
uart_available_for_write(uart_id_t uart) {
  TEENSY_UART_VALIDATE_ID(uart, 0)
  TEENSY_UART_DEVICE_SWITCH(uart_available_for_write_device, uart, 0);
}

/** Waits up to @p us microseconds for at least one byte to arrive.
 * Busy-waits with the DWT-based time_micros() clock; requires that
 * time_init() has run.
 * @param uart One of the uart1..uart8 constants.
 * @param us Maximum wait in microseconds (0 = instantaneous check).
 * @return true if a byte was received within the window, else false.
 */
#if defined(__clang__)
static inline bool uart_is_readable_within_us(uart_id_t uart, uint32_t us)
    __attribute__((diagnose_if(uart < UART_ID_1 || uart > UART_ID_8,
                               "invalid Teensy UART; expected uart1 through "
                               "uart8",
                               "error")));
#endif
static inline __attribute__((always_inline)) bool
uart_is_readable_within_us(uart_id_t uart, uint32_t us) {
  TEENSY_UART_VALIDATE_ID(uart, false)
  TEENSY_UART_DEVICE_SWITCH(uart_is_readable_within_us_device, uart, false, us);
}

#endif
