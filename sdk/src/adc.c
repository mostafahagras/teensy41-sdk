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

/* ---- interrupt-on-complete handlers ---- */
static adc_complete_handler_t adc_irq_handlers[2];
static void *adc_irq_contexts[2];
static const IMXRT_ADCS_t *const adc_irq_adcs[2] = {&IMXRT_ADC1, &IMXRT_ADC2};

static void adc_irq_handler1(void) {
  IMXRT_ADCS_t *adc = (IMXRT_ADCS_t *)adc_irq_adcs[0];
  adc_complete_handler_t handler = adc_irq_handlers[0];
  if ((adc->HS & ADC_HS_COCO0) != 0u && handler != NULL) {
    uint16_t value = (uint16_t)adc->R0; /* read clears COCO0 */
    handler(value, adc_irq_contexts[0]);
  }
}

static void adc_irq_handler2(void) {
  IMXRT_ADCS_t *adc = (IMXRT_ADCS_t *)adc_irq_adcs[1];
  adc_complete_handler_t handler = adc_irq_handlers[1];
  if ((adc->HS & ADC_HS_COCO0) != 0u && handler != NULL) {
    uint16_t value = (uint16_t)adc->R0;
    handler(value, adc_irq_contexts[1]);
  }
}

/* Arms one conversion that completes through the ADC interrupt (ADC_IE
 * set in the command register). */
int adc_trigger_impl(uint8_t instance, uint8_t channel) {
  IMXRT_ADCS_t *adc;

  if (instance < 1u || instance > 2u || channel > 15u || !adc_initialized)
    return -1;
  /* unpopulated channels float: reject the pair at runtime too. */
  if ((((instance == 1u) ? ADC1_CHANNELS_MASK : ADC2_CHANNELS_MASK) &
       (1u << channel)) == 0u)
    return -1;
  adc = (IMXRT_ADCS_t *)adc_irq_adcs[instance - 1u];
  adc->HC0 = ADC_HC_AIEN | ADC_HC_ADCH(channel);
  return 0;
}

int adc_attach_irq_impl(uint8_t instance,
                        void (*handler)(uint16_t value, void *context),
                        void *context) {
  if (instance < 1u || instance > 2u)
    return -1;
  adc_irq_handlers[instance - 1u] = handler;
  adc_irq_contexts[instance - 1u] = context;
  if (handler == NULL) {
    /* detach: disarm the interrupt line as well */
    NVIC_DISABLE_IRQ(instance == 1u ? IRQ_ADC1 : IRQ_ADC2);
    return 0;
  }
  attachInterruptVector(instance == 1u ? IRQ_ADC1 : IRQ_ADC2,
                        instance == 1u ? adc_irq_handler1 : adc_irq_handler2);
  NVIC_SET_PRIORITY(instance == 1u ? IRQ_ADC1 : IRQ_ADC2, 128);
  NVIC_ENABLE_IRQ(instance == 1u ? IRQ_ADC1 : IRQ_ADC2);
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
