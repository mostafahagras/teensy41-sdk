#include <teensy/dma.h>
#include <teensy/imxrt.h>

#include <stddef.h>

#define DMA_CHANNEL_COUNT 16u
#define DMA_MAX_TRANSFER_SIZE 32767u /* CITER is 15 bits without elinks */
#define DMA_WAIT_LIMIT 100000000u

/* The DMAMUX register file exposes 32 muxed slots; only the first 16 map to
 * eDMA channels.  The mux words sit 32 bits apart, so index-by-word works. */
static volatile uint32_t *const dma_mux_regs = &DMAMUX_CHCFG0;

typedef volatile IMXRT_DMA_TCD_t *dma_tcd_t;

static volatile uint32_t dma_used_mask;
static dma_callback_t dma_callbacks[DMA_CHANNEL_COUNT];

static dma_tcd_t dma_tcd(uint8_t channel) { return &IMXRT_DMA_TCD[channel]; }

/* Completion ISR for every channel: a set bit in DMA_INT means that channel
 * finished its major loop. */
static void dma_dispatch_isr(void) {
  uint8_t i;

  for (i = 0; i < DMA_CHANNEL_COUNT; ++i) {
    void (*callback)(void) = dma_callbacks[i];

    if ((DMA_INT & (1u << i)) == 0u)
      continue;
    DMA_INT = 1u << i; /* W1C */
    if (callback != NULL)
      callback();
  }
}

int dma_channel_acquire(void) {
  uint8_t i;

  CCM_CCGR5 |= CCM_CCGR5_DMA(CCM_CCGR_ON);

  for (i = 0; i < DMA_CHANNEL_COUNT; ++i) {
    if ((dma_used_mask & (1u << i)) != 0u)
      continue;
    dma_used_mask |= 1u << i;
    dma_callbacks[i] = NULL;
    DMA_CERQ = i;      /* disable requests from a previous owner */
    DMA_CDNE = i;      /* clear a DONE flag left by that owner */
    DMA_INT = 1u << i; /* clear a stale completion interrupt */
    dma_tcd(i)->CSR = 0;
    dma_mux_regs[i] = 0; /* channel 0x0 while unconfigured */
    return (int)i;
  }
  return DMA_ERROR_NONE_FREE;
}

void dma_channel_release(uint8_t channel) {
  if (channel >= DMA_CHANNEL_COUNT || (dma_used_mask & (1u << channel)) == 0u)
    return;

  DMA_CERQ = channel;
  dma_mux_regs[channel] = 0;
  dma_callbacks[channel] = NULL;
  NVIC_DISABLE_IRQ(IRQ_DMA_CH0 + channel);
  dma_used_mask &= ~(1u << channel);
}

int dma_copy(uint8_t channel, void *destination, const void *source,
             uint32_t size, dma_callback_t callback, uint8_t priority) {
  dma_tcd_t tcd;

  if (channel >= DMA_CHANNEL_COUNT || (dma_used_mask & (1u << channel)) == 0u)
    return DMA_ERROR_INVALID;
  if (destination == NULL || source == NULL)
    return DMA_ERROR_INVALID;
  if (size == 0u || size > DMA_MAX_TRANSFER_SIZE)
    return DMA_ERROR_RANGE;

  CCM_CCGR5 |= CCM_CCGR5_DMA(CCM_CCGR_ON);
  tcd = dma_tcd(channel);

  /* Rewind any previous state on this channel. */
  DMA_CERQ = channel;
  DMA_CDNE = channel;
  DMA_INT = 1u << channel;

  /* One 8-bit move per request; the always-on mux drives the requests
   * back-to-back until CITER exhausts. */
  tcd->SADDR = source;
  tcd->SOFF = 1;
  tcd->ATTR = 0;
  tcd->NBYTES = 1;
  tcd->SLAST = -(int32_t)size;
  tcd->DADDR = destination;
  tcd->DOFF = 1;
  tcd->CITER = (uint16_t)size;
  tcd->DLASTSGA = -(int32_t)size;
  tcd->CSR = 0;
  tcd->BITER = (uint16_t)size;
  tcd->CSR = DMA_TCD_CSR_INTMAJOR | DMA_TCD_CSR_DREQ;

  dma_callbacks[channel] = callback;
  dma_mux_regs[channel] = DMAMUX_CHCFG_A_ON | DMAMUX_CHCFG_ENBL;

  NVIC_CLEAR_PENDING(IRQ_DMA_CH0 + channel);
  _VectorsRam[IRQ_DMA_CH0 + channel + 16] = dma_dispatch_isr;
  if (callback != NULL) {
    NVIC_SET_PRIORITY(IRQ_DMA_CH0 + channel, priority);
    NVIC_ENABLE_IRQ(IRQ_DMA_CH0 + channel);
  }

  DMA_SERQ = channel;
  return DMA_OK;
}

bool dma_done(uint8_t channel) {
  if (channel >= DMA_CHANNEL_COUNT || (dma_used_mask & (1u << channel)) == 0u)
    return false;
  return (dma_tcd(channel)->CSR & DMA_TCD_CSR_DONE) != 0u;
}

int dma_wait(uint8_t channel) {
  uint32_t wait = DMA_WAIT_LIMIT;

  if (channel >= DMA_CHANNEL_COUNT || (dma_used_mask & (1u << channel)) == 0u)
    return DMA_ERROR_INVALID;

  while ((dma_tcd(channel)->CSR & DMA_TCD_CSR_DONE) == 0u && wait-- != 0u) {
    if (DMA_ES != 0u) {
      DMA_CERR = channel;
      return DMA_ERROR_HW;
    }
    __asm volatile("nop");
  }
  if (DMA_ES != 0u) {
    DMA_CERR = channel;
    return DMA_ERROR_HW;
  }
  if ((dma_tcd(channel)->CSR & DMA_TCD_CSR_DONE) == 0u)
    return DMA_ERROR_TIMEOUT;
  return DMA_OK;
}
