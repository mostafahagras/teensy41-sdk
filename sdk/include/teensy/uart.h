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

int uart_init(uart_id_t uart, uint32_t baud_rate);
int uart_available(uart_id_t uart);
int uart_read(uart_id_t uart);
size_t uart_write(uart_id_t uart, const void *data, size_t length);
int uart_write_byte(uart_id_t uart, uint8_t byte);
void uart_flush(uart_id_t uart);

#endif
