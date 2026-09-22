#include <teensy/imxrt.h>
#include <teensy/nvram.h>

#include <stddef.h>
#include <string.h>

/*
 * Ported from Teensyduino's cores/teensy4/eeprom.c for this board: the
 * append-only log layout, the address decoder, and the FlexSPI command
 * choreography (LUT slots 60-63, quad page program, sector erase, status
 * polling with a FlexSPI soft-reset afterwards) are all its.  Differences:
 * a C API instead of the Arduino EEPROM.h equivalents, and a slightly
 * cleaner address space bound (NVRAM_SIZE = 16128 fits the decoder exactly).
 */

#define NVRAM_SECTOR_COUNT 63u
#define NVRAM_SECTOR_BYTES 4096u
#define NVRAM_RECORDS_PER_SECTOR (NVRAM_SECTOR_BYTES / 2u)
#define NVRAM_LOG_BASE 0x607C0000u

/* JESD216-style flash opcodes, same assignments as the Teensy core. */
#define FLASH_CMD_WRITE_ENABLE 0x06u
#define FLASH_CMD_QUAD_PAGE_PROGRAM 0x32u
#define FLASH_CMD_SECTOR_ERASE 0x20u
#define FLASH_CMD_READ_STATUS 0x05u

static uint16_t nvram_log_used[NVRAM_SECTOR_COUNT];
static bool nvram_initialized;

/* LUT helpers: exactly the macros of the same names in the Teensy core's
 * eeprom.c but renamed to fall in line with our imxrt.h spellings
 * (FLEXSPI_LUT_INSTRUCTION, FLEXSPI_LUT_OPCODE_*, FLEXSPI_LUT_NUM_PADS_*). */
#define NV_LUT0(opcode, pads, operand)                                         \
  FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand))
#define NV_LUT1(opcode, pads, operand)                                         \
  (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)) << 16)
#define NV_CMD_SDR FLEXSPI_LUT_OPCODE_CMD_SDR
#define NV_RADDR_SDR FLEXSPI_LUT_OPCODE_RADDR_SDR
#define NV_READ_SDR FLEXSPI_LUT_OPCODE_READ_SDR
#define NV_WRITE_SDR FLEXSPI_LUT_OPCODE_WRITE_SDR
#define NV_PINS1 FLEXSPI_LUT_NUM_PADS_1
#define NV_PINS4 FLEXSPI_LUT_NUM_PADS_4

/* Slower path layout note: this waits until the flash reports the last
 * command completed, and mirrors Teensy's: the FlexSPI AHB FIFO is reset
 * so stale prefetched data cannot be re-read, and IRQs are unblocked. */
static void nvram_flash_wait(void) {
  FLEXSPI_LUT60 = NV_LUT0(NV_CMD_SDR, NV_PINS1, FLASH_CMD_READ_STATUS) |
                  NV_LUT1(NV_READ_SDR, NV_PINS1, 1);
  FLEXSPI_LUT61 = 0;
  uint8_t status;
  do {
    FLEXSPI_IPRXFCR = FLEXSPI_IPRXFCR_CLRIPRXF;
    FLEXSPI_IPCR0 = 0;
    FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15u) | FLEXSPI_IPCR1_IDATSZ(1u);
    FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
    while ((FLEXSPI_INTR & FLEXSPI_INTR_IPCMDDONE) == 0u)
      ;
    FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;
    __asm volatile("" ::: "memory");
    status = *(volatile uint8_t *)&FLEXSPI_RFDR0;
  } while ((status & 1u) != 0u);
  FLEXSPI_MCR0 |= FLEXSPI_MCR0_SWRESET;
  while ((FLEXSPI_MCR0 & FLEXSPI_MCR0_SWRESET) != 0u)
    ;
  __enable_irq();
}

/* Writes bytes into already-erased (0xFF) flash. */
static void nvram_flash_write(uint32_t address, const void *data,
                              uint32_t length) {
  __disable_irq();
  FLEXSPI_LUTKEY = FLEXSPI_LUTKEY_VALUE;
  FLEXSPI_LUTCR = FLEXSPI_LUTCR_UNLOCK;
  FLEXSPI_IPCR0 = 0;
  FLEXSPI_LUT60 = NV_LUT0(NV_CMD_SDR, NV_PINS1, FLASH_CMD_WRITE_ENABLE);
  FLEXSPI_LUT61 = 0;
  FLEXSPI_LUT62 = 0;
  FLEXSPI_LUT63 = 0;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15u);
  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
  arm_dcache_delete((void *)address, length);
  while ((FLEXSPI_INTR & FLEXSPI_INTR_IPCMDDONE) == 0u)
    ;
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;

  FLEXSPI_LUT60 = NV_LUT0(NV_CMD_SDR, NV_PINS1, FLASH_CMD_QUAD_PAGE_PROGRAM) |
                  NV_LUT1(NV_RADDR_SDR, NV_PINS1, 24u);
  FLEXSPI_LUT61 = NV_LUT0(NV_WRITE_SDR, NV_PINS4, 1u);
  FLEXSPI_IPTXFCR = FLEXSPI_IPTXFCR_CLRIPTXF;
  FLEXSPI_IPCR0 = address & 0x00FFFFFFu;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15u) | FLEXSPI_IPCR1_IDATSZ(length);
  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;

  const uint8_t *src = (const uint8_t *)data;
  uint32_t remaining = length;
  uint32_t intr;
  while (((intr = FLEXSPI_INTR) & FLEXSPI_INTR_IPCMDDONE) == 0u) {
    if ((intr & FLEXSPI_INTR_IPTXWE) != 0u) {
      uint32_t chunk = remaining > 8u ? 8u : remaining;
      if (chunk > 0u) {
        memcpy((void *)&FLEXSPI_TFDR0, src, chunk);
        src += chunk;
        remaining -= chunk;
      }
      FLEXSPI_INTR = FLEXSPI_INTR_IPTXWE;
    }
  }
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE | FLEXSPI_INTR_IPTXWE;
  nvram_flash_wait();
}

/* Erases one 4 KiB sector. */
static void nvram_flash_erase_sector(uint32_t address) {
  __disable_irq();
  FLEXSPI_LUTKEY = FLEXSPI_LUTKEY_VALUE;
  FLEXSPI_LUTCR = FLEXSPI_LUTCR_UNLOCK;
  FLEXSPI_IPCR0 = 0;
  FLEXSPI_LUT60 = NV_LUT0(NV_CMD_SDR, NV_PINS1, FLASH_CMD_WRITE_ENABLE);
  FLEXSPI_LUT61 = 0;
  FLEXSPI_LUT62 = 0;
  FLEXSPI_LUT63 = 0;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15u);
  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
  arm_dcache_delete((void *)(address & 0xFFFFF000u), NVRAM_SECTOR_BYTES);
  while ((FLEXSPI_INTR & FLEXSPI_INTR_IPCMDDONE) == 0u)
    ;
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;

  FLEXSPI_LUT60 = NV_LUT0(NV_CMD_SDR, NV_PINS1, FLASH_CMD_SECTOR_ERASE) |
                  NV_LUT1(NV_RADDR_SDR, NV_PINS1, 24u);
  FLEXSPI_IPCR0 = address & 0x00FFF000u;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15u);
  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
  while ((FLEXSPI_INTR & FLEXSPI_INTR_IPCMDDONE) == 0u)
    ;
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;
  nvram_flash_wait();
}

int nvram_init(void) {
  uint32_t sector;

  CCM_CCGR6 |= CCM_CCGR6_FLEXSPI(CCM_CCGR_ON);

  for (sector = 0; sector < NVRAM_SECTOR_COUNT; ++sector) {
    const volatile uint16_t *p =
        (const volatile uint16_t *)(NVRAM_LOG_BASE +
                                    sector * NVRAM_SECTOR_BYTES);
    const volatile uint16_t *const end = p + NVRAM_RECORDS_PER_SECTOR;
    uint16_t used = 0;

    while (p < end) {
      if (*p++ == 0xFFFFu)
        break;
      ++used;
    }
    nvram_log_used[sector] = used;
  }
  nvram_initialized = true;
  return NVRAM_OK;
}

bool nvram_ready(void) { return nvram_initialized; }

/* Address decoder; equivalent to the Teensy core's mapping. */
static void nvram_decode(uint16_t address, uint32_t *sector, uint16_t *key) {
  uint32_t word = (uint32_t)(address >> 2);
  uint32_t row = word / NVRAM_SECTOR_COUNT;

  *sector = word % NVRAM_SECTOR_COUNT;
  *key = (uint16_t)(((uint16_t)(address & 3u)) | ((uint16_t)row << 2));
}

uint8_t nvram_read(uint16_t address) {
  uint32_t sector;
  uint16_t key;
  const volatile uint16_t *p;
  const volatile uint16_t *end;
  uint8_t data = 0xFFu;

  if (!nvram_initialized || address >= NVRAM_SIZE)
    return 0xFFu;
  nvram_decode(address, &sector, &key);
  p = (const volatile uint16_t *)(NVRAM_LOG_BASE + sector * NVRAM_SECTOR_BYTES);
  end = p + nvram_log_used[sector];
  while (p < end) {
    uint16_t entry = *p++;
    if ((entry & 0xFFu) == (uint16_t)key)
      data = (uint8_t)(entry >> 8);
  }
  return data;
}

bool nvram_valid(uint16_t address) { return address < NVRAM_SIZE; }

int nvram_write(uint16_t address, uint8_t value) {
  uint32_t sector;
  uint16_t key;
  const volatile uint16_t *start;
  const volatile uint16_t *end;
  uint16_t *p;
  uint8_t stored;

  if (address >= NVRAM_SIZE)
    return NVRAM_ERROR_RANGE;
  if (!nvram_initialized)
    nvram_init();

  nvram_decode(address, &sector, &key);
  start =
      (const volatile uint16_t *)(NVRAM_LOG_BASE + sector * NVRAM_SECTOR_BYTES);
  end = start + nvram_log_used[sector];

  stored = nvram_read(address);
  if (stored == value)
    return NVRAM_OK;

  if (nvram_log_used[sector] < NVRAM_RECORDS_PER_SECTOR) {
    /* Append-only fast path: still room in the sector log. */
    uint16_t entry = (uint16_t)((uint16_t)value << 8 | (uint16_t)key);
    nvram_flash_write((uint32_t)end, &entry, sizeof(entry));
    nvram_log_used[sector] = (uint16_t)(nvram_log_used[sector] + 1u);
  } else {
    /* Consolidate: rebuild a compressed copy (last value per key), erase
     * the sector, rewrite one entry per non-default key. */
    uint8_t buffer[256];
    uint16_t index = 0;
    uint16_t i;
    uint16_t *p = (uint16_t *)start;
    const volatile uint16_t *scan = start;

    memset(buffer, 0xFFu, sizeof(buffer));
    while (scan != end) {
      uint16_t entry = *scan++;
      buffer[entry & 0xFFu] = (uint8_t)(entry >> 8);
    }
    buffer[key] = value;

    nvram_flash_erase_sector((uint32_t)start);
    for (i = 0; i < sizeof(buffer); ++i) {
      uint16_t entry = *p++;
      buffer[entry & 0xFFu] = (uint8_t)(entry >> 8);
    }
    buffer[key] = value;

    nvram_flash_erase_sector((uint32_t)start);
    /* Sector erase followed by writes to a fresh address range */
    for (i = 0; i < sizeof(buffer); ++i) {
      if (buffer[i] == 0xFFu)
        continue;
      uint16_t entry = (uint16_t)((uint16_t)buffer[i] << 8 | i);
      nvram_flash_write((uint32_t)start + index * sizeof(entry), &entry,
                        sizeof(entry));
      ++index;
    }
    nvram_log_used[sector] = index;
  }
  return NVRAM_OK;
}

void nvram_read_block(void *destination, uint16_t address, uint32_t length) {
  uint8_t *out = (uint8_t *)destination;

  while (length > 0u) {
    uint16_t slice =
        (uint16_t)(address + length > NVRAM_SIZE ? NVRAM_SIZE - address
                                                 : length);
    uint16_t i;

    for (i = 0; i < slice; ++i)
      out[i] = nvram_read((uint16_t)(address + i));
    out += slice;
    length -= slice;
    address = (uint16_t)(address + slice);
    if (address >= NVRAM_SIZE)
      break;
  }
}

int nvram_write_block(const void *source, uint16_t address, uint32_t length) {
  const uint8_t *in = (const uint8_t *)source;
  int status = NVRAM_OK;

  if (length == 0u)
    return NVRAM_OK;
  if (address >= NVRAM_SIZE || (uint32_t)address + length > NVRAM_SIZE)
    return NVRAM_ERROR_RANGE;
  if (!nvram_initialized)
    nvram_init();

  while (length > 0u) {
    status = nvram_write(address, *in++);
    if (status != NVRAM_OK)
      return status;
    ++address;
    --length;
  }
  return status;
}

void nvram_erase_all(void) {
  uint32_t sector;

  for (sector = 0; sector < NVRAM_SECTOR_COUNT; ++sector) {
    if (nvram_initialized && nvram_log_used[sector] == 0u)
      continue;
    nvram_flash_erase_sector(NVRAM_LOG_BASE + sector * NVRAM_SECTOR_BYTES);
    nvram_log_used[sector] = 0u;
  }
}
