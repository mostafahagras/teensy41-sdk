/* Persistent storage with the flash-backed NVRAM module.
 *
 * Byte-accurate storage on the reserved last flash sectors; writes
 * survive power loss and re-flashes.  This demo keeps a boot counter
 * (increments across every reset) and stores/reads back a short blob.
 */

#include <teensy/gpio.h>
#include <teensy/printf.h>
#include <teensy/nvram.h>
#include <teensy/time.h>
#include <teensy/usb.h>

#define BOOT_COUNTER_ADDRESS 5u
#define BLOB_ADDRESS 32u

int main(void) {
  uint8_t previous;
  uint8_t next;

  usb_init();
  gpio_configure(13, GPIO_OUTPUT);

  if (nvram_init() != NVRAM_OK) {
    puts("nvram init failed");
    return 1;
  }

  /* Boot counter: proves persistence across resets. */
  previous = nvram_read(BOOT_COUNTER_ADDRESS);
  next = previous == 0xFFu ? 1u : (uint8_t)(previous + 1u);
  if (nvram_write(BOOT_COUNTER_ADDRESS, next) != NVRAM_OK) {
    puts("nvram counter write failed");
    return 1;
  }
  printf("boots: previous=%02u now=%02u\r\n", previous, next);

  uint8_t blob[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x42, 0x00, 0x11, 0x22};
  (void)nvram_write_block(blob, BLOB_ADDRESS, sizeof(blob));
  {
    uint8_t confirm[8];
    nvram_read_block(confirm, BLOB_ADDRESS, sizeof(confirm));
    bool same = true;
    for (uint32_t i = 0; i < sizeof(confirm); ++i)
      if (confirm[i] != blob[i]) {
        same = false;
        printf("nvram blob mismatch at %lu\r\n", (unsigned long)i);
        break;
      }
    printf("blob write/read => %s\r\n", same ? "OK" : "FAIL");
  }

  gpio_toggle(13);
  while (1)
    time_delay_ms(1000);
}
