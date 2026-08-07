#ifndef TEENSY_USB_H
#define TEENSY_USB_H

#include <stddef.h>
#include <stdint.h>

void usb_init(void);
int usb_connected(void);
int usb_available(void);
int usb_read(void);
size_t usb_write(const void *data, size_t length);
int usb_write_byte(uint8_t byte);
void usb_flush(void);

#endif
