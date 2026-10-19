#include <teensy/cache.h>
#include <teensy/firmware_update.h>
#include <teensy/imxrt.h>
#include <teensy/time.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FW_FLASH_BASE 0x60000000u
#define FW_FLASH_SIZE 0x00800000u
#define FW_FLASH_LIMIT 0x607C0000u
#define FW_FLASH_SECTOR_SIZE 4096u
#define FW_FLASH_PAGE_SIZE 256u
#define FW_RAW_CHUNK_SIZE FW_FLASH_SECTOR_SIZE
#define FW_RAW_CHUNK_TIMEOUT_MS 2000u
#define FW_RX_TIMEOUT_MS_DEFAULT 60000u
#define FW_PROTOCOL_VERSION 1u
#define FW_PACKET_HEADER_SIZE 16u
#define FW_PACKET_MAX_PAYLOAD 256u
#define FW_PACKET_MAX_SIZE (FW_PACKET_HEADER_SIZE + FW_PACKET_MAX_PAYLOAD + 4u)

enum {
  FW_PACKET_BEGIN = 1,
  FW_PACKET_END = 3,
  FW_PACKET_ABORT = 4,
  FW_PACKET_RAW_CHUNK = 10,
  FW_PACKET_HELLO = 0x81,
  FW_PACKET_READY = 0x82,
  FW_PACKET_ACK = 0x83,
  FW_PACKET_NAK = 0x84,
  FW_PACKET_DONE = 0x85,
  FW_PACKET_REQUEST = 0x86,
  FW_PACKET_ERROR = 0xFF,
};

enum {
  FW_REMOTE_BAD_REQUEST = 1,
  FW_REMOTE_BAD_IMAGE = 2,
  FW_REMOTE_NO_SPACE = 3,
  FW_REMOTE_FLASH_FAILURE = 4,
  FW_REMOTE_ABORTED = 5,
  FW_REMOTE_FLASH_PROGRAM_RANGE = 0x10,
  FW_REMOTE_FLASH_PROGRAM_WRITE_ENABLE = 0x11,
  FW_REMOTE_FLASH_PROGRAM_COMMAND = 0x12,
  FW_REMOTE_FLASH_PROGRAM_READY = 0x13,
  FW_REMOTE_FLASH_PROGRAM_VERIFY = 0x14,
  FW_REMOTE_FLASH_PROTECTION = 0x15,
  FW_REMOTE_RX_TIMEOUT = 6,
};

enum {
  FW_PROTECTION_REGISTER_READ = 1,
  FW_PROTECTION_UNSUPPORTED_MODE = 2,
  FW_PROTECTION_UNSUPPORTED_FLASH = 3,
  FW_PROTECTION_STAGING_RANGE = 4,
  FW_PROTECTION_IMAGE_RANGE = 5,
  FW_PROTECTION_ACTIVE_IMAGE_RANGE = 6,
};

typedef struct {
  uint8_t type;
  uint32_t sequence;
  uint32_t offset;
  uint16_t length;
  uint8_t payload[FW_PACKET_MAX_PAYLOAD];
} fw_packet_t;

extern uint32_t _flashimagelen;

/* Referenced by the scanner so the marker survives linker section GC. */
static const char fw_target_marker[] = "TEENSY41-CSDK-FW1";

static uint32_t irq_save(void) {
  uint32_t primask;
  __asm volatile("mrs %0, primask\ncpsid i" : "=r"(primask) : : "memory");
  return primask;
}

static void irq_restore(uint32_t primask) {
  if ((primask & 1u) == 0u)
    __enable_irq();
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t length) {
  size_t i;
  for (i = 0; i < length; ++i) {
    uint32_t bit;
    crc ^= data[i];
    for (bit = 0; bit < 8u; ++bit)
      crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(crc & 1u));
  }
  return crc;
}

static uint32_t crc32(const uint8_t *data, size_t length) {
  return crc32_update(0xFFFFFFFFu, data, length) ^ 0xFFFFFFFFu;
}

static uint32_t read_u32_le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static void write_u16_le(uint8_t *p, uint16_t value) {
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
}

static void write_u32_le(uint8_t *p, uint32_t value) {
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

static bool uart_read_byte_timeout(uart_id_t uart, uint8_t *out,
                                   uint32_t timeout_ms) {
  uint32_t start = time_micros();
  uint32_t timeout_us = timeout_ms * 1000u;
  for (;;) {
    int byte = uart_read(uart);
    if (byte >= 0) {
      *out = (uint8_t)byte;
      return true;
    }
    if ((uint32_t)(time_micros() - start) >= timeout_us)
      return false;
  }
}

static bool uart_write_all(uart_id_t uart, const uint8_t *data, size_t length) {
  size_t offset = 0;
  while (offset < length) {
    size_t wrote = uart_write(uart, data + offset, length - offset);
    if (wrote == 0u) {
      if (uart_available_for_write(uart) == 0)
        continue;
      return false;
    }
    offset += wrote;
  }
  uart_flush(uart);
  return true;
}

static bool send_packet(uart_id_t uart, uint8_t type, uint32_t sequence,
                        uint32_t offset, const uint8_t *payload,
                        uint16_t length) {
  uint8_t bytes[FW_PACKET_MAX_SIZE];
  uint32_t sum;
  if (length > FW_PACKET_MAX_PAYLOAD)
    return false;
  memcpy(bytes, "TUF1", 4u);
  bytes[4] = FW_PROTOCOL_VERSION;
  bytes[5] = type;
  write_u32_le(bytes + 6u, sequence);
  write_u32_le(bytes + 10u, offset);
  write_u16_le(bytes + 14u, length);
  if (length != 0u && payload != NULL)
    memcpy(bytes + FW_PACKET_HEADER_SIZE, payload, length);
  sum = crc32(bytes, FW_PACKET_HEADER_SIZE + length);
  write_u32_le(bytes + FW_PACKET_HEADER_SIZE + length, sum);
  return uart_write_all(uart, bytes, FW_PACKET_HEADER_SIZE + length + 4u);
}

static bool read_packet(uart_id_t uart, fw_packet_t *packet,
                        uint32_t timeout_ms, bool *bad_crc) {
  uint8_t matched = 0u;
  uint8_t byte;
  uint8_t frame[FW_PACKET_MAX_SIZE];
  uint16_t used = 0u;
  uint16_t frame_length = 0u;
  uint32_t start = time_micros();
  uint32_t last_byte = start;

  *bad_crc = false;
  for (;;) {
    if (!uart_read_byte_timeout(uart, &byte, 10u)) {
      uint32_t now = time_micros();
      /* A partial frame that stops arriving is abandoned. This lets the
       * sender's retransmission re-establish framing instead of making the
       * updater wait for the bad header's claimed payload length. */
      if (used != 0u && (uint32_t)(now - last_byte) >= 100000u) {
        used = 0u;
        frame_length = 0u;
        matched = 0u;
      }
      if ((uint32_t)(now - start) >= timeout_ms * 1000u)
        return false;
      continue;
    }
    last_byte = time_micros();

    if (used == 0u) {
      if (matched == 0u && byte == FW_UPDATE_REQUEST_BYTE) {
        packet->type = FW_PACKET_REQUEST;
        packet->sequence = 0u;
        packet->offset = 0u;
        packet->length = 0u;
        return true;
      }
      if (byte == (uint8_t)"TUF1"[matched]) {
        ++matched;
        if (matched == 4u) {
          memcpy(frame, "TUF1", 4u);
          used = 4u;
          matched = 0u;
        }
      } else {
        matched = byte == 'T' ? 1u : 0u;
      }
      continue;
    }

    frame[used++] = byte;
    if (used == FW_PACKET_HEADER_SIZE) {
      uint16_t length = (uint16_t)frame[14] | ((uint16_t)frame[15] << 8);
      if (frame[4] != FW_PROTOCOL_VERSION || length > FW_PACKET_MAX_PAYLOAD) {
        /* Do not abandon update mode on a damaged header. The host will
         * retry the frame; resume scanning for its next magic. */
        used = 0u;
        frame_length = 0u;
        matched = 0u;
        continue;
      }
      frame_length = (uint16_t)(FW_PACKET_HEADER_SIZE + length + 4u);
    }
    if (frame_length != 0u && used == frame_length) {
      uint32_t expected = read_u32_le(frame + frame_length - 4u);
      uint32_t actual = crc32(frame, frame_length - 4u);
      packet->type = frame[5];
      packet->sequence = read_u32_le(frame + 6u);
      packet->offset = read_u32_le(frame + 10u);
      packet->length = (uint16_t)frame[14] | ((uint16_t)frame[15] << 8);
      if (packet->length != 0u)
        memcpy(packet->payload, frame + FW_PACKET_HEADER_SIZE, packet->length);
      *bad_crc = expected != actual;
      return true;
    }

    if (used >= sizeof(frame)) {
      used = 0u;
      frame_length = 0u;
      matched = 0u;
    }
  }
}

static bool flexspi_wait_command(uint32_t timeout_ms) {
  uint32_t start = time_micros();
  while ((FLEXSPI_INTR & FLEXSPI_INTR_IPCMDDONE) == 0u) {
    if ((uint32_t)(time_micros() - start) >= timeout_ms * 1000u)
      return false;
  }
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;
  return true;
}

static bool flash_wait_ready(uint32_t timeout_ms) {
  uint32_t start = time_micros();
  FLEXSPI_LUT60 = FLEXSPI_LUT_INSTRUCTION(FLEXSPI_LUT_OPCODE_CMD_SDR,
                                          FLEXSPI_LUT_NUM_PADS_1, 0x05u) |
                  (FLEXSPI_LUT_INSTRUCTION(FLEXSPI_LUT_OPCODE_READ_SDR,
                                           FLEXSPI_LUT_NUM_PADS_1, 1u)
                   << 16);
  FLEXSPI_LUT61 = 0u;
  do {
    FLEXSPI_IPRXFCR = FLEXSPI_IPRXFCR_CLRIPRXF;
    FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;
    FLEXSPI_IPCR0 = 0u;
    FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15u) | FLEXSPI_IPCR1_IDATSZ(1u);
    FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
    if (!flexspi_wait_command(100u))
      return false;
    __asm volatile("" ::: "memory");
    /* Status register bit 0 is WIP/BUSY. Other status bits may be set
     * independently, so don't require the entire status byte to be zero. */
    uint8_t status = ((uint8_t *)&FLEXSPI_RFDR0)[0];
    if ((status & 1u) == 0u)
      break;
    if ((uint32_t)(time_micros() - start) >= timeout_ms * 1000u)
      return false;
  } while (true);

  FLEXSPI_MCR0 |= FLEXSPI_MCR0_SWRESET;
  while ((FLEXSPI_MCR0 & FLEXSPI_MCR0_SWRESET) != 0u) {
    if ((uint32_t)(time_micros() - start) >= timeout_ms * 1000u)
      return false;
  }
  return true;
}

static bool flash_read_register(uint8_t command, uint8_t *data,
                                uint8_t length) {
  uint32_t i;
  if (data == NULL || length == 0u || length > 4u)
    return false;
  FLEXSPI_LUTKEY = FLEXSPI_LUTKEY_VALUE;
  FLEXSPI_LUTCR = FLEXSPI_LUTCR_UNLOCK;
  FLEXSPI_IPRXFCR = FLEXSPI_IPRXFCR_CLRIPRXF;
  FLEXSPI_LUT60 = FLEXSPI_LUT_INSTRUCTION(FLEXSPI_LUT_OPCODE_CMD_SDR,
                                          FLEXSPI_LUT_NUM_PADS_1, command) |
                  (FLEXSPI_LUT_INSTRUCTION(FLEXSPI_LUT_OPCODE_READ_SDR,
                                           FLEXSPI_LUT_NUM_PADS_1, length)
                   << 16);
  FLEXSPI_LUT61 = 0u;
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;
  FLEXSPI_IPCR0 = 0u;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15u) | FLEXSPI_IPCR1_IDATSZ(length);
  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
  if (!flexspi_wait_command(100u))
    return false;
  for (i = 0u; i < length; ++i)
    data[i] = ((volatile uint8_t *)&FLEXSPI_RFDR0)[i];
  return true;
}

static bool flash_write_enable(void) {
  FLEXSPI_LUTKEY = FLEXSPI_LUTKEY_VALUE;
  FLEXSPI_LUTCR = FLEXSPI_LUTCR_UNLOCK;
  FLEXSPI_LUT60 = FLEXSPI_LUT_INSTRUCTION(FLEXSPI_LUT_OPCODE_CMD_SDR,
                                          FLEXSPI_LUT_NUM_PADS_1, 0x06u);
  FLEXSPI_LUT61 = 0u;
  FLEXSPI_LUT62 = 0u;
  FLEXSPI_LUT63 = 0u;
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;
  FLEXSPI_IPCR0 = 0u;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15u);
  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
  return flexspi_wait_command(100u);
}

/* Wait for FlexSPI to become idle, then select the IP-bus TX FIFO window used
 * by the SDK's NVRAM writer. */
static bool flash_select_ip_tx_fifo(void) {
  uint32_t start = time_micros();
  while ((FLEXSPI_STS0 & (FLEXSPI_STS0_ARBIDLE | FLEXSPI_STS0_SEQIDLE)) !=
         (FLEXSPI_STS0_ARBIDLE | FLEXSPI_STS0_SEQIDLE)) {
    if ((uint32_t)(time_micros() - start) >= 100000u)
      return false;
  }
  FLEXSPI_MCR0 &= ~FLEXSPI_MCR0_ATDFEN;
  return (FLEXSPI_MCR0 & FLEXSPI_MCR0_ATDFEN) == 0u;
}

/* W25Q64JV's SR1/SR2 block-protection map. Return true only when a protected
 * byte intersects the requested range. The updater reserves the last 256 KiB
 * for NVRAM and never writes there, so an unrelated top-of-flash protection
 * must not prevent updates. */
static bool flash_range_is_protected(uint32_t start, uint32_t size,
                                     uint8_t status1, uint8_t status2) {
  uint32_t bp = (status1 >> 2) & 7u;
  uint32_t protected_size;
  uint32_t protected_start;
  uint32_t end = start + size;
  uint32_t protected_end;
  bool cmp = (status2 & 0x40u) != 0u;
  bool tb = (status1 & 0x20u) != 0u;

  if (size == 0u || end < start || start < FW_FLASH_BASE ||
      end > FW_FLASH_BASE + FW_FLASH_SIZE)
    return true;
  if (bp == 0u)
    protected_size = 0u;
  else if (bp == 7u)
    protected_size = FW_FLASH_SIZE;
  else if ((status1 & 0x40u) != 0u)
    protected_size = FW_FLASH_SECTOR_SIZE << (bp - 1u);
  else
    protected_size = FW_FLASH_SIZE >> (7u - bp);

  protected_start =
      tb ? FW_FLASH_BASE : FW_FLASH_BASE + FW_FLASH_SIZE - protected_size;
  protected_end = protected_start + protected_size;
  if (!cmp)
    return protected_size != 0u && start < protected_end &&
           end > protected_start;

  /* CMP protects the complement of the BP/TB/SEC-selected range. */
  return protected_size == 0u || start < protected_start || end > protected_end;
}

static bool flash_prepare_for_update(uint32_t stage_start, uint32_t image_size,
                                     uint32_t *reason) {
  uint8_t jedec_id[3];
  uint8_t status1;
  uint8_t status2;
  uint8_t status3;
  uint32_t active_size = (uint32_t)(uintptr_t)&_flashimagelen;

  if (!flash_read_register(0x9Fu, jedec_id, sizeof(jedec_id)) ||
      !flash_read_register(0x05u, &status1, 1u) ||
      !flash_read_register(0x35u, &status2, 1u) ||
      !flash_read_register(0x15u, &status3, 1u)) {
    if (reason != NULL)
      *reason = FW_PROTECTION_REGISTER_READ;
    return false;
  }

  /* EF 70 17 identifies the 8 MiB Winbond W25Q64 family fitted to Teensy
   * 4.1. Do not guess at protection semantics for an unknown flash device. */
  if (jedec_id[0] != 0xEFu || jedec_id[1] != 0x70u || jedec_id[2] != 0x17u ||
      (status3 & 0x04u) != 0u) {
    if (reason != NULL)
      *reason = (status3 & 0x04u) != 0u ? FW_PROTECTION_UNSUPPORTED_MODE
                                        : FW_PROTECTION_UNSUPPORTED_FLASH;
    return false;
  }

  if (flash_range_is_protected(stage_start, image_size, status1, status2)) {
    if (reason != NULL)
      *reason = FW_PROTECTION_STAGING_RANGE;
    return false;
  }
  if (flash_range_is_protected(FW_FLASH_BASE, image_size, status1, status2)) {
    if (reason != NULL)
      *reason = FW_PROTECTION_IMAGE_RANGE;
    return false;
  }
  if (flash_range_is_protected(FW_FLASH_BASE, active_size, status1, status2)) {
    if (reason != NULL)
      *reason = FW_PROTECTION_ACTIVE_IMAGE_RANGE;
    return false;
  }
  if (reason != NULL)
    *reason = 0u;
  return true;
}

static bool flash_erase_sector(uint32_t address) {
  uint32_t primask = irq_save();
  bool result = false;

  if (address < FW_FLASH_BASE || address >= FW_FLASH_LIMIT ||
      (address & (FW_FLASH_SECTOR_SIZE - 1u)) != 0u)
    goto done;
  if (!flash_write_enable())
    goto done;

  arm_dcache_delete((void *)address, FW_FLASH_SECTOR_SIZE);
  FLEXSPI_LUT60 = FLEXSPI_LUT_INSTRUCTION(FLEXSPI_LUT_OPCODE_CMD_SDR,
                                          FLEXSPI_LUT_NUM_PADS_1, 0x20u) |
                  (FLEXSPI_LUT_INSTRUCTION(FLEXSPI_LUT_OPCODE_RADDR_SDR,
                                           FLEXSPI_LUT_NUM_PADS_1, 24u)
                   << 16);
  FLEXSPI_LUT61 = 0u;
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE;
  FLEXSPI_IPCR0 = address & 0x00FFF000u;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15u);
  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
  if (!flexspi_wait_command(1000u) || !flash_wait_ready(10000u))
    goto done;
  result = true;

done:
  irq_restore(primask);
  if (result) {
    const volatile uint32_t *words = (const volatile uint32_t *)address;
    uint32_t i;
    for (i = 0u; i < FW_FLASH_SECTOR_SIZE / sizeof(*words); ++i) {
      if (words[i] != UINT32_MAX)
        return false;
    }
  }
  return result;
}

static bool flash_program_page(uint32_t address, const uint8_t *data,
                               uint16_t length, uint32_t *failure_code) {
  uint32_t primask = irq_save();
  uint32_t start = time_micros();
  bool result = false;
  uint32_t remaining = length;
  const uint8_t *source = data;

  if (data == NULL || length == 0u || length > FW_FLASH_PAGE_SIZE ||
      address < FW_FLASH_BASE || address + length > FW_FLASH_LIMIT ||
      (address & (FW_FLASH_PAGE_SIZE - 1u)) + length > FW_FLASH_PAGE_SIZE) {
    if (failure_code != NULL)
      *failure_code = FW_REMOTE_FLASH_PROGRAM_RANGE;
    goto done;
  }
  if (!flash_select_ip_tx_fifo()) {
    if (failure_code != NULL)
      *failure_code = FW_REMOTE_FLASH_PROGRAM_COMMAND;
    goto done;
  }
  if (!flash_write_enable()) {
    if (failure_code != NULL)
      *failure_code = FW_REMOTE_FLASH_PROGRAM_WRITE_ENABLE;
    goto done;
  }
  /* Teensyduino goes directly from WREN to page program. */
  arm_dcache_delete((void *)address, length);
  FLEXSPI_LUT60 = FLEXSPI_LUT_INSTRUCTION(FLEXSPI_LUT_OPCODE_CMD_SDR,
                                          FLEXSPI_LUT_NUM_PADS_1, 0x32u) |
                  (FLEXSPI_LUT_INSTRUCTION(FLEXSPI_LUT_OPCODE_RADDR_SDR,
                                           FLEXSPI_LUT_NUM_PADS_1, 24u)
                   << 16);
  FLEXSPI_LUT61 = FLEXSPI_LUT_INSTRUCTION(FLEXSPI_LUT_OPCODE_WRITE_SDR,
                                          FLEXSPI_LUT_NUM_PADS_4, 1u);
  /* Match the known-good Teensy/NVRAM FIFO sequence: default watermark and
   * two 32-bit TFDR writes per request. */
  FLEXSPI_IPTXFCR = FLEXSPI_IPTXFCR_CLRIPTXF;
  /* The WREN wait above already clears IPCMDDONE. Preserve IPTXWE here: the
   * Teensyduino/NVRAM sequence consumes the pending FIFO-ready indication
   * after triggering page program. Clearing it before the command can lose
   * the first opportunity to feed TFDR. */
  FLEXSPI_IPCR0 = address & 0x00FFFFFFu;
  FLEXSPI_IPCR1 = FLEXSPI_IPCR1_ISEQID(15u) | FLEXSPI_IPCR1_IDATSZ(length);

  FLEXSPI_IPCMD = FLEXSPI_IPCMD_TRG;
  for (;;) {
    uint32_t intr = FLEXSPI_INTR;
    if ((intr & (FLEXSPI_INTR_IPCMDERR | FLEXSPI_INTR_IPCMDGE |
                 FLEXSPI_INTR_SEQTIMEOUT)) != 0u) {
      FLEXSPI_INTR = intr & (FLEXSPI_INTR_IPCMDERR | FLEXSPI_INTR_IPCMDGE |
                             FLEXSPI_INTR_SEQTIMEOUT);
      if (failure_code != NULL)
        *failure_code = FW_REMOTE_FLASH_PROGRAM_COMMAND;
      goto done;
    }
    /* IPCMDDONE takes priority over IPTXWE. Once the controller reports the
     * sequence complete, bytes written in response to a stale watermark flag
     * would remain in the FIFO and could be mistaken for a successful write. */
    if ((intr & FLEXSPI_INTR_IPCMDDONE) != 0u)
      break;
    if ((intr & FLEXSPI_INTR_IPTXWE) != 0u) {
      if (remaining != 0u) {
        uint32_t chunk = remaining > 8u ? 8u : remaining;
        /* The SDK memcpy implementation uses byte stores, which the
         * FlexSPI TFDR register window ignores. Write complete FIFO words. */
        volatile uint32_t *fifo = (volatile uint32_t *)&FLEXSPI_TFDR0;
        uint32_t word;
        for (word = 0u; word * sizeof(uint32_t) < chunk; ++word) {
          uint32_t word_offset = word * sizeof(uint32_t);
          uint32_t bytes = chunk - word_offset;
          uint32_t value = UINT32_MAX;
          uint32_t byte;
          if (bytes > sizeof(uint32_t))
            bytes = sizeof(uint32_t);
          for (byte = 0u; byte < bytes; ++byte) {
            uint32_t shift = byte * 8u;
            value = (value & ~(0xFFu << shift)) |
                    ((uint32_t)source[word_offset + byte] << shift);
          }
          fifo[word] = value;
        }
        __asm volatile("dsb sy" ::: "memory");
        source += chunk;
        remaining -= chunk;
      }
      FLEXSPI_INTR = FLEXSPI_INTR_IPTXWE;
    }
    if ((uint32_t)(time_micros() - start) >= 1000000u) {
      if (failure_code != NULL)
        *failure_code = FW_REMOTE_FLASH_PROGRAM_COMMAND;
      goto done;
    }
  }
  FLEXSPI_INTR = FLEXSPI_INTR_IPCMDDONE | FLEXSPI_INTR_IPTXWE;
  __asm volatile("dsb sy" ::: "memory");
  if (remaining != 0u) {
    if (failure_code != NULL)
      *failure_code = FW_REMOTE_FLASH_PROGRAM_COMMAND;
    goto done;
  }
  if (!flash_wait_ready(10000u)) {
    if (failure_code != NULL)
      *failure_code = FW_REMOTE_FLASH_PROGRAM_READY;
    goto done;
  }
  result = true;

done:
  irq_restore(primask);
  if (result && failure_code != NULL)
    *failure_code = 0u;
  return result;
}

static bool flash_write_staged(uint32_t address, const uint8_t *data,
                               uint16_t length, uint32_t *failure_code,
                               uint32_t *mismatch_offset,
                               uint32_t *expected_value,
                               uint32_t *actual_value) {
  uint8_t page[FW_FLASH_PAGE_SIZE];
  uint32_t aligned_length = (uint32_t)((length + 3u) & ~3u);
  if (length == 0u || aligned_length > FW_FLASH_PAGE_SIZE ||
      (address & (FW_FLASH_PAGE_SIZE - 1u)) != 0u) {
    if (failure_code != NULL)
      *failure_code = FW_REMOTE_FLASH_PROGRAM_RANGE;
    return false;
  }
  memset(page, 0xFF, sizeof(page));
  memcpy(page, data, length);
  {
    uint32_t offset;
    if (!flash_program_page(address, page, (uint16_t)aligned_length,
                            failure_code))
      return false;
    arm_dcache_delete((void *)address, aligned_length);
    for (offset = 0u; offset < aligned_length; ++offset) {
      {
        const volatile uint8_t *flash_bytes =
            (const volatile uint8_t *)(address + offset);
        if (flash_bytes[0] != page[offset]) {
          if (failure_code != NULL)
            *failure_code = FW_REMOTE_FLASH_PROGRAM_VERIFY;
          if (mismatch_offset != NULL)
            *mismatch_offset = offset;
          if (expected_value != NULL)
            *expected_value = page[offset];
          if (actual_value != NULL)
            *actual_value = flash_bytes[0];
          return false;
        }
      }
    }
  }
  return true;
}

static bool flash_erase_range(uint32_t start, uint32_t size) {
  uint32_t end = start + size;
  uint32_t address;
  for (address = start; address < end; address += FW_FLASH_SECTOR_SIZE) {
    if (!flash_erase_sector(address))
      return false;
  }
  return true;
}

static bool image_has_target_marker(uint32_t address, uint32_t length) {
  const uint8_t *image = (const uint8_t *)address;
  const size_t marker_length = sizeof(fw_target_marker) - 1u;
  uint32_t i;
  if (length < marker_length)
    return false;
  for (i = 0u; i <= length - marker_length; ++i) {
    if (memcmp(image + i, fw_target_marker, marker_length) == 0)
      return true;
  }
  return false;
}

static __attribute__((noreturn)) void activation_failure_reset(void) {
  SCB_AIRCR = 0x05FA0004u;
  __asm volatile("dsb" ::: "memory");
  for (;;)
    __asm volatile("wfi");
}

static __attribute__((noreturn)) void activate_image(uint32_t stage,
                                                     uint32_t image_size) {
  uint8_t page[FW_FLASH_PAGE_SIZE];
  uint32_t active_size = (uint32_t)(uintptr_t)&_flashimagelen;
  uint32_t erase_size = active_size > image_size ? active_size : image_size;
  uint32_t erase_rounded =
      (erase_size + FW_FLASH_SECTOR_SIZE - 1u) & ~(FW_FLASH_SECTOR_SIZE - 1u);
  uint32_t offset;

  /* Activation is terminal. Keep interrupts masked while the active image is
   * being changed; all code and constants used here live in ITCM/DTCM. */
  (void)irq_save();
  for (offset = 0u; offset < erase_rounded; offset += FW_FLASH_SECTOR_SIZE) {
    if (!flash_erase_sector(FW_FLASH_BASE + offset))
      activation_failure_reset();
    if (offset < image_size) {
      uint32_t page_offset;
      uint32_t sector_image_bytes = image_size - offset;
      if (sector_image_bytes > FW_FLASH_SECTOR_SIZE)
        sector_image_bytes = FW_FLASH_SECTOR_SIZE;
      for (page_offset = 0u; page_offset < sector_image_bytes;
           page_offset += FW_FLASH_PAGE_SIZE) {
        uint32_t remaining = sector_image_bytes - page_offset;
        uint16_t length =
            (uint16_t)(remaining > FW_FLASH_PAGE_SIZE ? FW_FLASH_PAGE_SIZE
                                                      : remaining);
        uint16_t write_length = (uint16_t)((length + 3u) & ~3u);
        uint32_t copy_offset = offset + page_offset;
        memset(page, 0xFF, sizeof(page));
        memcpy(page, (const void *)(stage + copy_offset), length);
        if (!flash_program_page(FW_FLASH_BASE + copy_offset, page, write_length,
                                NULL))
          activation_failure_reset();
      }
    }
  }

  __asm volatile("dsb\nisb" ::: "memory");
  SCB_AIRCR = 0x05FA0004u;
  __asm volatile("dsb" ::: "memory");
  for (;;)
    __asm volatile("wfi");
}

static fw_update_result_t
send_remote_error(uart_id_t uart, fw_update_result_t result, uint32_t code) {
  uint8_t payload[4];
  write_u32_le(payload, code);
  (void)send_packet(uart, FW_PACKET_ERROR, 0u, 0u, payload, sizeof(payload));
  return result;
}

static fw_update_result_t send_remote_protection_error(uart_id_t uart,
                                                       uint32_t reason) {
  uint8_t payload[8];
  write_u32_le(payload, FW_REMOTE_FLASH_PROTECTION);
  write_u32_le(payload + 4u, reason);
  (void)send_packet(uart, FW_PACKET_ERROR, 0u, 0u, payload, sizeof(payload));
  return FW_UPDATE_ERROR_FLASH;
}

static fw_update_result_t send_remote_flash_verify_error(uart_id_t uart,
                                                         uint32_t offset,
                                                         uint32_t expected,
                                                         uint32_t actual) {
  uint8_t payload[16];
  write_u32_le(payload, FW_REMOTE_FLASH_PROGRAM_VERIFY);
  write_u32_le(payload + 4u, offset);
  write_u32_le(payload + 8u, expected);
  write_u32_le(payload + 12u, actual);
  (void)send_packet(uart, FW_PACKET_ERROR, 0u, 0u, payload, sizeof(payload));
  return FW_UPDATE_ERROR_FLASH;
}

static fw_update_result_t
fw_update_uart_run_internal(uart_id_t uart,
                            const fw_update_options_t *options) {
  uart_rx_lease_t lease = {0};
  fw_packet_t packet;
  uint8_t chunk[FW_RAW_CHUNK_SIZE];
  uint8_t hello[4];
  uint8_t ready[8];
  uint32_t timeout_ms = options != NULL && options->frame_timeout_ms != 0u
                            ? options->frame_timeout_ms
                            : FW_RX_TIMEOUT_MS_DEFAULT;
  uint32_t active_end = FW_FLASH_BASE + (uint32_t)(uintptr_t)&_flashimagelen;
  uint32_t stage_start =
      (active_end + FW_FLASH_SECTOR_SIZE - 1u) & ~(FW_FLASH_SECTOR_SIZE - 1u);
  uint32_t capacity =
      stage_start < FW_FLASH_LIMIT ? FW_FLASH_LIMIT - stage_start : 0u;
  uint32_t image_size = 0u;
  uint32_t expected_crc = 0u;
  uint32_t stream_crc = 0xFFFFFFFFu;
  uint32_t received = 0u;
  uint32_t expected_sequence = 0u;
  uint32_t last_sequence = UINT32_MAX;
  uint32_t last_offset = 0u;
  uint32_t last_length = 0u;
  uint32_t last_chunk_crc = 0u;
  uint32_t flash_bytes;
  bool bad_crc;
  fw_update_result_t result = FW_UPDATE_ERROR_PROTOCOL;

  if (uart < UART_ID_1 || uart > UART_ID_8 || uart_claim_rx(uart, &lease) != 0)
    return FW_UPDATE_ERROR_UART;
  uart_clear(uart);
  write_u32_le(hello, capacity);
  if (!send_packet(uart, FW_PACKET_HELLO, 0u, 0u, hello, sizeof(hello))) {
    result = FW_UPDATE_ERROR_UART;
    goto raw_release;
  }

  for (;;) {
    if (!read_packet(uart, &packet, timeout_ms, &bad_crc)) {
      result = send_remote_error(uart, FW_UPDATE_ERROR_TIMEOUT,
                                 FW_REMOTE_RX_TIMEOUT);
      goto raw_release;
    }
    if (bad_crc) {
      (void)send_packet(uart, FW_PACKET_NAK, packet.sequence, packet.offset,
                        NULL, 0u);
      continue;
    }
    if (packet.type == FW_PACKET_REQUEST) {
      (void)send_packet(uart, FW_PACKET_HELLO, 0u, 0u, hello, sizeof(hello));
      continue;
    }
    if (packet.type == FW_PACKET_BEGIN && packet.length == 8u &&
        packet.sequence == 0u && packet.offset == 0u)
      break;
    (void)send_packet(uart, FW_PACKET_NAK, 0u, 0u, NULL, 0u);
  }

  image_size = read_u32_le(packet.payload);
  expected_crc = read_u32_le(packet.payload + 4u);
  if (image_size == 0u || image_size > capacity ||
      image_size > FW_FLASH_LIMIT - FW_FLASH_BASE) {
    result =
        send_remote_error(uart, FW_UPDATE_ERROR_CAPACITY, FW_REMOTE_NO_SPACE);
    goto raw_release;
  }
  {
    uint32_t protection_reason;
    if (!flash_prepare_for_update(stage_start, image_size,
                                  &protection_reason)) {
      result = send_remote_protection_error(uart, protection_reason);
      goto raw_release;
    }
  }
  flash_bytes =
      (image_size + FW_FLASH_SECTOR_SIZE - 1u) & ~(FW_FLASH_SECTOR_SIZE - 1u);
  if (!flash_erase_range(stage_start, flash_bytes)) {
    result =
        send_remote_error(uart, FW_UPDATE_ERROR_FLASH, FW_REMOTE_FLASH_FAILURE);
    goto raw_release;
  }
  write_u32_le(ready, FW_RAW_CHUNK_SIZE);
  write_u32_le(ready + 4u, capacity);
  if (!send_packet(uart, FW_PACKET_READY, 0u, 0u, ready, sizeof(ready))) {
    result = FW_UPDATE_ERROR_UART;
    goto raw_release;
  }

  while (received < image_size) {
    uint32_t chunk_length;
    uint32_t chunk_crc;
    uint32_t chunk_size;
    uint32_t chunk_offset;
    bool duplicate;
    if (!read_packet(uart, &packet, timeout_ms, &bad_crc)) {
      result = send_remote_error(uart, FW_UPDATE_ERROR_TIMEOUT,
                                 FW_REMOTE_RX_TIMEOUT);
      goto raw_release;
    }
    if (bad_crc) {
      (void)send_packet(uart, FW_PACKET_NAK, packet.sequence, packet.offset,
                        NULL, 0u);
      continue;
    }
    if (packet.type == FW_PACKET_ABORT) {
      result =
          send_remote_error(uart, FW_UPDATE_ERROR_PROTOCOL, FW_REMOTE_ABORTED);
      goto raw_release;
    }
    if (packet.type != FW_PACKET_RAW_CHUNK || packet.length != 8u) {
      (void)send_packet(uart, FW_PACKET_NAK, packet.sequence, packet.offset,
                        NULL, 0u);
      continue;
    }

    chunk_length = read_u32_le(packet.payload);
    chunk_crc = read_u32_le(packet.payload + 4u);
    duplicate = packet.sequence == last_sequence &&
                packet.offset == last_offset && chunk_length == last_length &&
                chunk_crc == last_chunk_crc;
    if (duplicate) {
      chunk_size = last_length;
      chunk_offset = last_offset;
    } else {
      uint32_t remaining = image_size - received;
      uint32_t expected_length =
          remaining > FW_RAW_CHUNK_SIZE ? FW_RAW_CHUNK_SIZE : remaining;
      if (packet.sequence != expected_sequence || packet.offset != received ||
          chunk_length != expected_length || chunk_length == 0u ||
          chunk_length > sizeof(chunk)) {
        (void)send_packet(uart, FW_PACKET_NAK, packet.sequence, packet.offset,
                          NULL, 0u);
        continue;
      }
      chunk_size = chunk_length;
      chunk_offset = received;
    }

    {
      uint32_t index;
      bool complete = true;
      for (index = 0u; index < chunk_size; ++index) {
        if (!uart_read_byte_timeout(uart, &chunk[index],
                                    FW_RAW_CHUNK_TIMEOUT_MS)) {
          complete = false;
          break;
        }
      }
      if (!complete || crc32(chunk, chunk_size) != chunk_crc) {
        (void)send_packet(uart, FW_PACKET_NAK, packet.sequence, packet.offset,
                          NULL, 0u);
        continue;
      }
    }

    if (duplicate) {
      if (memcmp((const void *)(stage_start + chunk_offset), chunk,
                 chunk_size) != 0) {
        result =
            send_remote_error(uart, FW_UPDATE_ERROR_IMAGE, FW_REMOTE_BAD_IMAGE);
        goto raw_release;
      }
      if (!send_packet(uart, FW_PACKET_ACK, packet.sequence, packet.offset,
                       NULL, 0u)) {
        result = FW_UPDATE_ERROR_UART;
        goto raw_release;
      }
      continue;
    }

    {
      uint32_t page_offset;
      for (page_offset = 0u; page_offset < chunk_size;
           page_offset += FW_FLASH_PAGE_SIZE) {
        uint32_t remaining = chunk_size - page_offset;
        uint16_t page_length =
            (uint16_t)(remaining > FW_FLASH_PAGE_SIZE ? FW_FLASH_PAGE_SIZE
                                                      : remaining);
        uint32_t failure_code = FW_REMOTE_FLASH_FAILURE;
        uint32_t mismatch_offset = 0u;
        uint32_t expected_value = 0u;
        uint32_t actual_value = 0u;
        if (!flash_write_staged(stage_start + chunk_offset + page_offset,
                                chunk + page_offset, page_length, &failure_code,
                                &mismatch_offset, &expected_value,
                                &actual_value)) {
          if (failure_code == FW_REMOTE_FLASH_PROGRAM_VERIFY) {
            result = send_remote_flash_verify_error(
                uart, chunk_offset + page_offset + mismatch_offset,
                expected_value, actual_value);
          } else {
            result =
                send_remote_error(uart, FW_UPDATE_ERROR_FLASH, failure_code);
          }
          goto raw_release;
        }
      }
    }
    stream_crc = crc32_update(stream_crc, chunk, chunk_size);
    last_sequence = packet.sequence;
    last_offset = packet.offset;
    last_length = chunk_size;
    last_chunk_crc = chunk_crc;
    received += chunk_size;
    ++expected_sequence;
    if (!send_packet(uart, FW_PACKET_ACK, packet.sequence, packet.offset, NULL,
                     0u)) {
      result = FW_UPDATE_ERROR_UART;
      goto raw_release;
    }
  }

  for (;;) {
    if (!read_packet(uart, &packet, timeout_ms, &bad_crc)) {
      result = send_remote_error(uart, FW_UPDATE_ERROR_TIMEOUT,
                                 FW_REMOTE_RX_TIMEOUT);
      goto raw_release;
    }
    if (!bad_crc && packet.type == FW_PACKET_END && packet.length == 0u &&
        packet.sequence == expected_sequence && packet.offset == image_size)
      break;
    if (!bad_crc && packet.type == FW_PACKET_RAW_CHUNK && packet.length == 8u &&
        packet.sequence == last_sequence && packet.offset == last_offset &&
        read_u32_le(packet.payload) == last_length &&
        read_u32_le(packet.payload + 4u) == last_chunk_crc) {
      uint32_t index;
      bool complete = true;
      for (index = 0u; index < last_length; ++index) {
        if (!uart_read_byte_timeout(uart, &chunk[index],
                                    FW_RAW_CHUNK_TIMEOUT_MS)) {
          complete = false;
          break;
        }
      }
      if (complete && crc32(chunk, last_length) == last_chunk_crc &&
          memcmp((const void *)(stage_start + last_offset), chunk,
                 last_length) == 0) {
        if (!send_packet(uart, FW_PACKET_ACK, last_sequence, last_offset, NULL,
                         0u)) {
          result = FW_UPDATE_ERROR_UART;
          goto raw_release;
        }
        continue;
      }
      (void)send_packet(uart, FW_PACKET_NAK, packet.sequence, packet.offset,
                        NULL, 0u);
      continue;
    }
    if (bad_crc || packet.type != FW_PACKET_END || packet.length != 0u ||
        packet.sequence != expected_sequence || packet.offset != image_size) {
      (void)send_packet(uart, FW_PACKET_NAK, packet.sequence, packet.offset,
                        NULL, 0u);
      continue;
    }
  }
  if ((stream_crc ^ 0xFFFFFFFFu) != expected_crc ||
      crc32((const uint8_t *)stage_start, image_size) != expected_crc ||
      !image_has_target_marker(stage_start, image_size)) {
    result =
        send_remote_error(uart, FW_UPDATE_ERROR_IMAGE, FW_REMOTE_BAD_IMAGE);
    goto raw_release;
  }
  if (!send_packet(uart, FW_PACKET_DONE, packet.sequence, image_size, NULL,
                   0u)) {
    result = FW_UPDATE_ERROR_UART;
    goto raw_release;
  }
  activate_image(stage_start, image_size);
  result = FW_UPDATE_OK;

raw_release:
  uart_clear(uart);
  uart_release_rx(&lease);
  return result;
}

fw_update_result_t fw_update_uart_run(uart_id_t uart,
                                      const fw_update_options_t *options) {
  return fw_update_uart_run_internal(uart, options);
}
