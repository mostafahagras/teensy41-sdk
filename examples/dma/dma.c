/* DMA memory-to-memory copies, three channels in flight simultaneously.
 *
 * The allocator leases exclusive channels; the three copies race the
 * buffers, every callback fires, then the capacity check acquires all
 * remaining channels and expects -1 when they are exhausted.
 */

#include <teensy/dma.h>
#include <teensy/printf.h>
#include <teensy/time.h>
#include <teensy/usb.h>

#define SIZE_A 4096u
#define SIZE_B 2048u
#define SIZE_C 30000u

static uint8_t src_a[SIZE_A], dst_a[SIZE_A];
static uint8_t src_b[SIZE_B], dst_b[SIZE_B];
static uint8_t src_c[SIZE_C], dst_c[SIZE_C];

static volatile uint32_t completions;

static void done_isr(void) { ++completions; }

static uint32_t rng_state = 2166136261u;

static void fill(uint8_t *buf, uint32_t size) {
  for (uint32_t i = 0; i < size; ++i) {
    rng_state = rng_state * 1664525u + 1013904223u;
    buf[i] = (uint8_t)(rng_state >> 24);
  }
}

static bool same(const uint8_t *a, const uint8_t *b, uint32_t size) {
  for (uint32_t i = 0; i < size; ++i)
    if (a[i] != b[i]) return false;
  return true;
}

int main(void) {
  usb_init();
  while (!usb_connected())
    time_delay_ms(100);
  time_delay_ms(300);

  int c1 = dma_channel_acquire();
  int c2 = dma_channel_acquire();
  int c3 = dma_channel_acquire();
  if (c1 < 0 || c2 < 0 || c3 < 0) {
    puts("acquire failed");
    return 1;
  }

  fill(src_a, SIZE_A);
  fill(src_b, SIZE_B);
  fill(src_c, SIZE_C);

  int s1 = dma_copy((uint8_t)c1, dst_a, src_a, SIZE_A, done_isr, 48);
  int s2 = dma_copy((uint8_t)c2, dst_b, src_b, SIZE_B, done_isr, 48);
  int s3 = dma_copy((uint8_t)c3, dst_c, src_c, SIZE_C, done_isr, 48);
  printf("copies => %d %d %d\r\n", s1, s2, s3);

  (void)dma_wait((uint8_t)c1);
  (void)dma_wait((uint8_t)c2);
  (void)dma_wait((uint8_t)c3);
  printf("completions=%lu a=%d b=%d c=%d\r\n", (unsigned long)completions,
         (int)same(dst_a, src_a, SIZE_A), (int)same(dst_b, src_b, SIZE_B),
         (int)same(dst_c, src_c, SIZE_C));

  int extra = 0;
  for (;;) {
    int ch = dma_channel_acquire();
    if (ch < 0) break;
    ++extra;
  }
  printf("drain => %d more, next => %d\r\n", extra,
         dma_channel_acquire());

  dma_channel_release((uint8_t)c1);
  dma_channel_release((uint8_t)c2);
  dma_channel_release((uint8_t)c3);
  puts("dma example done");
  while (1)
    time_delay_ms(1000);
}
