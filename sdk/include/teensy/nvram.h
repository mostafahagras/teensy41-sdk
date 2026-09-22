#ifndef TEENSY_NVRAM_H
#define TEENSY_NVRAM_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Persistent byte store, emulated with append-only logs on the last 63
 * 4 KiB sectors of the QSPI flash (0x607C0000, outside the program image;
 * the technique is the same as Teensyduino's EEPROM emulation for this
 * board).  Each sector stores 2048 records of one data byte tagged by one
 * "offset" key; nvram_write() appends a record and consolidates (erase and
 * rewrite) a sector once its log is full.
 *
 * Writes program the flash, which takes milliseconds and is crash-unsafe
 * for interleaved flash execution: both are inherited from how the FlexSPI
 * controller is driven here (matching the Teensyduino core).
 *
 * The address decoder below spreads the 4-byte groups of the address
 * space across all sectors, so rewriting one location does not endlessly
 * cycle a single sector:
 *   sector = (address >> 2) % 63
 *   key    = (address & 3) | ((address >> 2) / 63 << 2)
 */

/** Persistent storage size in bytes (63 sectors x 64 rows x 4 bytes). */
#define NVRAM_SIZE (63u * 64u * 4u)

enum {
  NVRAM_OK = 0,
  NVRAM_ERROR_RANGE = -1, /* address outside 0..NVRAM_SIZE-1 (or length) */
};

/**
 * Scans the flash sectors to recover log positions.
 *
 * Written data survives resets and re-flashes automatically; calling this
 * again is harmless.
 *
 * @return NVRAM_OK on success, or a nvram error code on failure.
 */
int nvram_init(void);

/** Returns whether nvram_init() has completed successfully. */
bool nvram_ready(void);

/**
 * Reads one byte.  Addresses never written return 0xFF.
 * @return The stored byte, or 0xFF for out-of-range addresses.
 */
uint8_t nvram_read(uint16_t address);

/** Returns whether an address is inside the supported range. */
bool nvram_valid(uint16_t address);

/**
 * Writes one byte; a redundant write of the same value is a no-op.
 * Appends a record, or consolidates the whole sector when its log is
 * full.  Blocks for the flash programming time.
 *
 * @return NVRAM_OK on success, or a nvram error code on failure.
 */
int nvram_write(uint16_t address, uint8_t value);

/** Reads @p length bytes into @p destination. */
void nvram_read_block(void *destination, uint16_t address, uint32_t length);

/**
 * Writes @p length bytes from @p source.  Bytes already containing the
 * exact stored values are skipped, so re-writing an unchanged block is
 * cheap.
 *
 * @return NVRAM_OK on success, or a nvram error code on failure.
 */
int nvram_write_block(const void *source, uint16_t address, uint32_t length);

/** Erases every storage sector; all nvram_read() calls return 0xFF after. */
void nvram_erase_all(void);

#endif
