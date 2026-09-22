#ifndef TEENSY_DMA_H
#define TEENSY_DMA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * eDMA on the RT1062, with the 16-channel DMAMUX in front of it.  This is
 * the memory-to-memory slice: byte-granular copies driven by the mux's
 * always-on request source, one completion callback per channel.
 *
 * Channels are exclusive: acquire one, run any number of dma_copy()
 * transfers on it, then release it.  Each copy is a one-shot; when CITER
 * exhausts, hardware clears the request (CSR DREQ) and raises the major
 * interrupt so the callback fires and the channel goes idle.
 *
 * Transfers are limited to 32767 bytes per call by the 15-bit CITER.
 *
 * Cache note: DTCM (the default home of .data/.bss) is not cached, so DMA
 * writes are CPU-visible there without maintenance.  For buffers in AXI
 * SRAM (0x202xxxxx), flush the source and invalidate the destination
 * around the transfer; the cache module provides the helpers.
 */

enum {
  DMA_OK = 0,
  DMA_ERROR_NONE_FREE = -1, /* dma_channel_acquire() found no free channel */
  DMA_ERROR_INVALID = -2,   /* unknown/unacquired channel or bad arguments */
  DMA_ERROR_RANGE = -3,     /* size outside representable range          */
  DMA_ERROR_TIMEOUT = -4,   /* dma_wait() gave up waiting                */
  DMA_ERROR_HW = -5,        /* eDMA reported a transfer error (DMA_ES)   */
};

typedef void (*dma_callback_t)(void);

/**
 * Claims a free eDMA channel (0-15).
 * @return The channel number, or DMA_ERROR_NONE_FREE when all are in use.
 */
int dma_channel_acquire(void);

/** Releases a channel.  Safe on unknown or already-free channels. */
void dma_channel_release(uint8_t channel);

/**
 * Starts an interrupt-on-completion byte copy of @p size bytes.  The size
 * limit per transfer is 32767 bytes (the 15-bit CITER field).
 *
 * @param callback Runs once in interrupt context after the copy; pass NULL
 *                 to rely on polling with dma_wait() instead.
 * @return DMA_OK on success, or a dma error code on failure.
 */
int dma_copy(uint8_t channel, void *destination, const void *source,
             uint32_t size, dma_callback_t callback, uint8_t priority);

/** Returns whether the channel's current transfer has finished. */
bool dma_done(uint8_t channel);

/**
 * Busy-waits until the channel completes or times out.
 * Errors are reported on first sight, not after the wait.
 * @return DMA_OK, DMA_ERROR_TIMEOUT, or DMA_ERROR_HW.
 */
int dma_wait(uint8_t channel);

#endif
