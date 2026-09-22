/* True random numbers from the TRNG.
 *
 * trng_init() powers up the ring-oscillator entropy source with NXP's
 * characterized tuning and enforces the hardware statistical checks on
 * every 512-bit word; the two prints show the block API and the
 * single-word convenience API.
 */

#include <teensy/printf.h>
#include <teensy/time.h>
#include <teensy/trng.h>
#include <teensy/usb.h>

static void print_hex_bytes(const uint8_t *data, uint32_t count) {
  for (uint32_t i = 0; i < count; ++i)
    printf("%02x", data[i]);
}

int main(void) {
  usb_init();

  if (trng_init() != TRNG_OK) {
    puts("trng init failed");
    return 1;
  }

  uint32_t words[4];
  (void)trng_read(words, sizeof(words));
  printf("rng words: %08lX %08lX %08lX %08lX\r\n", (unsigned long)words[0],
         (unsigned long)words[1], (unsigned long)words[2],
         (unsigned long)words[3]);

  uint8_t bytes[16];
  (void)trng_read(bytes, sizeof(bytes));
  printf("rng bytes: ");
  print_hex_bytes(bytes, sizeof(bytes));
  printf("\r\n");

  while (1)
    time_delay_ms(1000);
}
