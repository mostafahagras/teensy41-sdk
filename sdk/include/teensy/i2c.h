#ifndef TEENSY_I2C_H
#define TEENSY_I2C_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
  I2C_ID_INVALID = 0,
  I2C_ID_1 = 1,
  I2C_ID_2,
  I2C_ID_3,
  I2C_ID_4,
  I2C_COUNT = 5
} i2c_id_t;

#define i2c1 ((i2c_id_t)I2C_ID_1)
#define i2c2 ((i2c_id_t)I2C_ID_2)
#define i2c3 ((i2c_id_t)I2C_ID_3)
#define i2c4 ((i2c_id_t)I2C_ID_4)

enum {
  I2C_OK = 0,
  I2C_ERROR_INVALID = -1,
  I2C_ERROR_TIMEOUT = -2,
  I2C_ERROR_NACK = -3,
  I2C_ERROR_ARBITRATION = -4,
  I2C_ERROR_FIFO = -5
};

/** Initializes an I2C controller as a master at the requested bus frequency.
 * @return I2C_OK on success, or an I2C error code on failure.
 */
int i2c_init(i2c_id_t bus, uint32_t frequency_hz);

/** Writes bytes to a 7-bit I2C device address.
 * @return I2C_OK on success, or an I2C error code on failure.
 */
int i2c_write(i2c_id_t bus, uint8_t address, const void *data, size_t length);

/** Reads bytes from a 7-bit I2C device address.
 * @return I2C_OK on success, or an I2C error code on failure.
 */
int i2c_read(i2c_id_t bus, uint8_t address, void *data, size_t length);

/** Writes and then reads in one transaction using a repeated start.
 * @return I2C_OK on success, or an I2C error code on failure.
 */
int i2c_write_read(i2c_id_t bus, uint8_t address, const void *write_data,
                   size_t write_length, void *read_data, size_t read_length);

#endif
