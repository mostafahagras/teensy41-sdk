#ifndef TEENSY_FIRMWARE_UPDATE_H
#define TEENSY_FIRMWARE_UPDATE_H

#include <stdint.h>
#include <teensy/uart.h>

#define FW_UPDATE_REQUEST_BYTE 0x55u

typedef enum {
  FW_UPDATE_OK = 0,
  FW_UPDATE_ERROR_ARGUMENT = -1,
  FW_UPDATE_ERROR_UART = -2,
  FW_UPDATE_ERROR_TIMEOUT = -3,
  FW_UPDATE_ERROR_PROTOCOL = -4,
  FW_UPDATE_ERROR_IMAGE = -5,
  FW_UPDATE_ERROR_CAPACITY = -6,
  FW_UPDATE_ERROR_FLASH = -7,
} fw_update_result_t;

typedef struct {
  /* Timeout while waiting for host data. The update receiver defaults to 60 s.
   */
  uint32_t frame_timeout_ms;
} fw_update_options_t;

/**
 * Receives a firmware image over an initialized UART, stages and verifies it,
 * copies it over the active image, then resets. The caller must first stop
 * application code that transmits on @p uart. RX callbacks are temporarily
 * claimed and restored if this function returns with an error.
 *
 * The host starts this routine by sending FW_UPDATE_REQUEST_BYTE (0x55).
 * Image data is sent as raw UART bytes in sector-sized chunks. Each chunk is
 * CRC-checked and acknowledged after it has been staged and read back. The
 * complete staged image is checked before activation. After the request, this
 * function owns the UART until error or reset and does not require a
 * firmware-sized RAM buffer.
 *
 * @param uart One of uart1..uart8, already initialized for 115200 8N1.
 * @param options Optional timeout settings; NULL selects defaults.
 * @return FW_UPDATE_ERROR_* on failure. Successful activation resets and does
 *         not return.
 */
fw_update_result_t fw_update_uart_run(uart_id_t uart,
                                      const fw_update_options_t *options);

#endif
