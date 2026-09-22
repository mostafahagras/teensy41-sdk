#include <teensy/clock.h>
#include <teensy/gpt.h>
#include <teensy/imxrt.h>

#include <stddef.h>

#define GPT_COMPARE_COUNT 3u

/* Clock source 1 is the "peripheral clock" = ipg_perclk, whose source the
 * CCM CSCMR1[PERCLK_CLK_SEL] mux selects - exactly like the PIT's count
 * clock.  clock_init() routes that mux to the 24 MHz crystal, so the GPT
 * ticks at 24 MHz; the helper below follows the mux like timer.c does.
 * (Source 5 + EN_24M also nominally reaches the crystal but proved dead on
 * this board, matching the worked-around variants in manitou48/teensy4
 * examples.) */
#define GPT_CLKSRC_PERIPH 1u

static uint32_t gpt_hz(void) {
  if ((CCM_CSCMR1 & CCM_CSCMR1_PERCLK_CLK_SEL) != 0u)
    return 24000000u;
  return clock_bus_frequency_hz;
}

static uint32_t gpt_ticks_per_us(void) { return gpt_hz() / 1000000u; }

/* GPT word indices into the 32-bit register file: CR, PR, SR, IR, then
 * OCR1-3 at words 4-6, and CNT at word 9. */
#define GPT_WORD_CR 0u
#define GPT_WORD_PR 1u
#define GPT_WORD_SR 2u
#define GPT_WORD_IR 3u
#define GPT_WORD_OCR 4u
#define GPT_WORD_CNT 9u

typedef volatile IMXRT_REGISTER32_t *gpt_regs_t;

static gpt_regs_t gpt_bases[2] = {&IMXRT_GPT1, &IMXRT_GPT2};

typedef struct {
  void (*callback)(void);
  uint32_t period_ticks;
  uint32_t next_target;
  uint8_t priority;
} gpt_compare_state_t;

static gpt_compare_state_t gpt_states[2][GPT_COMPARE_COUNT];

static gpt_regs_t gpt_regs(gpt_timer_t timer) {
  return gpt_bases[(uint8_t)timer];
}

static volatile uint32_t *gpt_word(gpt_regs_t regs, uint32_t word) {
  return &((volatile uint32_t *)regs)[word];
}

/* Priority for a timer's shared interrupt: the most urgent attached
 * compare, or 255 when nothing is attached. */
static uint8_t gpt_top_priority(uint8_t timer) {
  uint8_t top = 255u;
  uint8_t c;

  for (c = 0; c < GPT_COMPARE_COUNT; ++c) {
    if (gpt_states[timer][c].callback != NULL &&
        gpt_states[timer][c].priority < top)
      top = gpt_states[timer][c].priority;
  }
  return top;
}

static void gpt_process(uint8_t timer) {
  gpt_regs_t regs = gpt_bases[timer];
  uint32_t status = *gpt_word(regs, GPT_WORD_SR);
  uint8_t c;

  for (c = 0; c < GPT_COMPARE_COUNT; ++c) {
    void (*callback)(void) = gpt_states[timer][c].callback;
    uint32_t flag = 1u << c;

    if ((status & flag) == 0u)
      continue;

    *gpt_word(regs, GPT_WORD_SR) = flag; /* W1C before user code */

    if (callback == NULL)
      continue;

    /* Keep the beats anchored to the schedule rather than to "now", so a
     * late callback does not stretch every later period. */
    gpt_states[timer][c].next_target += gpt_states[timer][c].period_ticks;
    if ((int32_t)(gpt_states[timer][c].next_target -
                  *gpt_word(regs, GPT_WORD_CNT)) <= 2)
      gpt_states[timer][c].next_target =
          *gpt_word(regs, GPT_WORD_CNT) + gpt_states[timer][c].period_ticks;
    *gpt_word(regs, GPT_WORD_OCR + c) = gpt_states[timer][c].next_target;

    callback();
  }
  /* The RT1062 GPT needs this barrier so the ISR does not re-enter on a
   * race between the flag clear above and the interrupt return below. */
  __asm volatile("dsb");
}

static void gpt1_isr(void) { gpt_process(0); }

static void gpt2_isr(void) { gpt_process(1); }

int gpt_init(void) {
  uint8_t t;
  uint8_t c;

  CCM_CCGR0 |=
      CCM_CCGR0_GPT2_BUS(CCM_CCGR_ON) | CCM_CCGR0_GPT2_SERIAL(CCM_CCGR_ON);
  CCM_CCGR1 |=
      CCM_CCGR1_GPT1_BUS(CCM_CCGR_ON) | CCM_CCGR1_GPT1_SERIAL(CCM_CCGR_ON);

  for (t = 0; t < 2u; ++t) {
    gpt_regs_t regs = gpt_bases[t];

    for (c = 0; c < GPT_COMPARE_COUNT; ++c) {
      gpt_states[t][c].callback = NULL;
      gpt_states[t][c].period_ticks = 0;
      gpt_states[t][c].next_target = 0;
      gpt_states[t][c].priority = 255u;
    }

    /* NXP initialization order: stop, software reset (self-clears),
     * clear stale status, disarm interrupts, disarm the compare slots,
     * then configure and run on the crystal. */
    *gpt_word(regs, GPT_WORD_CR) = 0;
    *gpt_word(regs, GPT_WORD_CR) |= GPT_CR_SWR;
    while ((*gpt_word(regs, GPT_WORD_CR) & GPT_CR_SWR) != 0u)
      __asm volatile("nop");
    *gpt_word(regs, GPT_WORD_CR) = 0;
    *gpt_word(regs, GPT_WORD_SR) = 0x3F;
    *gpt_word(regs, GPT_WORD_IR) = 0;
    *gpt_word(regs, GPT_WORD_PR) = 0;
    *gpt_word(regs, GPT_WORD_OCR) = 0xFFFFFFFFu;
    *gpt_word(regs, GPT_WORD_OCR + 1u) = 0xFFFFFFFFu;
    *gpt_word(regs, GPT_WORD_OCR + 2u) = 0xFFFFFFFFu;
    *gpt_word(regs, GPT_WORD_CR) = GPT_CR_FRR | GPT_CR_WAITEN |
                                   GPT_CR_CLKSRC(GPT_CLKSRC_PERIPH) | GPT_CR_EN;
  }
  return GPT_OK;
}

void gpt_stop(void) {
  uint8_t t;

  for (t = 0; t < 2u; ++t)
    *gpt_word(gpt_bases[t], GPT_WORD_CR) = 0; /* frozen; CNT stays readable */
}

uint32_t gpt_counter(gpt_timer_t timer) {
  return *gpt_word(gpt_regs(timer), GPT_WORD_CNT);
}

uint32_t gpt_elapsed_us(gpt_timer_t timer, uint32_t ticks_then) {
  uint32_t ticks = gpt_ticks_per_us();
  uint32_t delta = gpt_counter(timer) - ticks_then;

  return (delta + ticks / 2u) / ticks;
}

int gpt_attach_compare(gpt_timer_t timer, uint8_t compare, uint32_t period_us,
                       void (*callback)(void), uint8_t priority) {
  uint8_t t;
  uint8_t c;
  gpt_regs_t regs;
  uint32_t flag;
  uint32_t irq;

  if ((uint8_t)timer > 1u)
    return GPT_ERROR_TIMER;
  if (compare < 1u || compare > GPT_COMPARE_COUNT)
    return GPT_ERROR_COMPARE;
  t = (uint8_t)timer;
  c = compare - 1u;
  flag = 1u << c;
  regs = gpt_regs(timer);
  irq = t == 0u ? IRQ_GPT1 : IRQ_GPT2;

  if (callback != NULL) {
    uint64_t ticks64;

    if (period_us == 0u)
      return GPT_ERROR_RANGE;
    if (gpt_states[t][c].callback != NULL)
      return GPT_ERROR_BUSY;

    ticks64 = ((uint64_t)period_us * (uint64_t)gpt_hz() + 999999u) / 1000000u;
    if (ticks64 == 0u || ticks64 > 0xFFFFFFFFull)
      return GPT_ERROR_RANGE;

    gpt_states[t][c].callback = callback;
    gpt_states[t][c].period_ticks = (uint32_t)ticks64;
    gpt_states[t][c].priority = priority;
    gpt_states[t][c].next_target = gpt_counter(timer) + (uint32_t)ticks64;
    *gpt_word(regs, GPT_WORD_OCR + c) = gpt_states[t][c].next_target;

    *gpt_word(regs, GPT_WORD_SR) = flag;
    *gpt_word(regs, GPT_WORD_IR) |= flag;

    NVIC_CLEAR_PENDING(irq);
    _VectorsRam[irq + 16] = t == 0u ? gpt1_isr : gpt2_isr;
    NVIC_SET_PRIORITY(irq, gpt_top_priority(t));
    NVIC_ENABLE_IRQ(irq);
    return GPT_OK;
  }

  /* Detach; leave the IRQ enabled if another compare on this timer is
   * armed. */
  gpt_states[t][c].callback = NULL;
  *gpt_word(regs, GPT_WORD_IR) &= ~flag;
  *gpt_word(regs, GPT_WORD_OCR + c) = 0xFFFFFFFFu;
  if (gpt_top_priority(t) != 255u)
    NVIC_SET_PRIORITY(irq, gpt_top_priority(t));
  else
    NVIC_DISABLE_IRQ(irq);
  return GPT_OK;
}

bool gpt_attached(gpt_timer_t timer, uint8_t compare) {
  if ((uint8_t)timer > 1u || compare < 1u || compare > GPT_COMPARE_COUNT)
    return false;
  return gpt_states[(uint8_t)timer][compare - 1u].callback != NULL;
}
