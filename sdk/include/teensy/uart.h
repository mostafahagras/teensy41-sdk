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

#define uart1 ((uart_id_t)UART_ID_1)
#define uart2 ((uart_id_t)UART_ID_2)
#define uart3 ((uart_id_t)UART_ID_3)
#define uart4 ((uart_id_t)UART_ID_4)
#define uart5 ((uart_id_t)UART_ID_5)
#define uart6 ((uart_id_t)UART_ID_6)
#define uart7 ((uart_id_t)UART_ID_7)
#define uart8 ((uart_id_t)UART_ID_8)

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

#endif
