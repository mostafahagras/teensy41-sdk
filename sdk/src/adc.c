#include <stdbool.h>
#include <stdint.h>

#include <teensy/clock.h>
#include <teensy/gpio.h>
#include <teensy/imxrt.h>

#define TEENSY_ADC_IMPLEMENTATION
#include <teensy/adc.h>

uint8_t adc_initialized;

static uint32_t adc_resolution_bits = 12;
static uint32_t adc_averaging_samples = 1;

static uint32_t adc_resolution_mode(uint32_t bits) {
  switch (bits) {
  case 8:
    return ADC_CFG_MODE(0) | ADC_CFG_ADSTS(3);
  case 10:
    return ADC_CFG_MODE(1) | ADC_CFG_ADSTS(2) | ADC_CFG_ADLSMP;
  default:
    return ADC_CFG_MODE(2) | ADC_CFG_ADSTS(3) | ADC_CFG_ADLSMP;
  }
}

static uint32_t adc_averaging_mode(uint32_t samples) {
  switch (samples) {
  case 32:
    return ADC_CFG_AVGS(3) | ADC_GC_AVGE;
  case 16:
    return ADC_CFG_AVGS(2) | ADC_GC_AVGE;
  case 8:
    return ADC_CFG_AVGS(1) | ADC_GC_AVGE;
  case 4:
    return ADC_CFG_AVGS(0) | ADC_GC_AVGE;
  default:
    return 0;
  }
}

static void adc_apply_config(IMXRT_ADCS_t *adc) {
  uint32_t mode = adc_resolution_mode(adc_resolution_bits) | ADC_CFG_ADIV(1) |
                  ADC_CFG_ADICLK(3) | ADC_CFG_ADHSC;
  adc->CFG = mode;
  adc->GC = adc_averaging_mode(adc_averaging_samples);
}

static int adc_calibrate(IMXRT_ADCS_t *adc) {
  uint32_t wait = 1000000u;

  adc->GC |= ADC_GC_CAL;
  while ((adc->GC & ADC_GC_CAL) != 0u && wait-- != 0u)
    __asm volatile("nop");
  if ((adc->GC & ADC_GC_CAL) != 0u || (adc->GS & ADC_GS_CALF) != 0u)
    return -1;
  return 0;
}

int adc_init(void) {
  CCM_CCGR1 |= CCM_CCGR1_ADC1(CCM_CCGR_ON) | CCM_CCGR1_ADC2(CCM_CCGR_ON);
  adc_apply_config(&IMXRT_ADC1);
  adc_apply_config(&IMXRT_ADC2);
  if (adc_calibrate(&IMXRT_ADC1) != 0 || adc_calibrate(&IMXRT_ADC2) != 0) {
    adc_initialized = 0;
    return -1;
  }
  adc_initialized = 1;
  return 0;
}

int adc_set_resolution_impl(uint32_t bits) {
  if (bits != 8u && bits != 10u && bits != 12u)
    return -1;
  adc_resolution_bits = bits;
  if (adc_initialized) {
    adc_apply_config(&IMXRT_ADC1);
    adc_apply_config(&IMXRT_ADC2);
  }
  return 0;
}

int adc_set_averaging_impl(uint32_t samples) {
  if (samples != 1u && samples != 4u && samples != 8u && samples != 16u &&
      samples != 32u)
    return -1;
  adc_averaging_samples = samples;
  if (adc_initialized) {
    adc_apply_config(&IMXRT_ADC1);
    adc_apply_config(&IMXRT_ADC2);
  }
  return 0;
}
