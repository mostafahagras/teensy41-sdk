#ifndef TEENSY_USB_H
#define TEENSY_USB_H

#include <stddef.h>
#include <stdint.h>

/** Initializes the USB device controller and CDC serial interface. */
void usb_init(void);

/** Returns nonzero after the host has configured the USB device. */
int usb_connected(void);

/** Returns the number of bytes waiting in the USB CDC receive buffer. */
int usb_available(void);

/** Reads one byte from USB CDC.
 * @return The byte as an unsigned value, or -1 if no byte is available.
 */
int usb_read(void);

/** Writes bytes to USB CDC.
 * @return The number of bytes accepted for transmission.
 */
size_t usb_write(const void *data, size_t length);

/** Writes one byte to USB CDC.
 * @return 1 on success, or 0 if the byte could not be queued.
 */
int usb_write_byte(uint8_t byte);

/** Requests immediate transmission of buffered USB CDC output. */
void usb_flush(void);

#endif
