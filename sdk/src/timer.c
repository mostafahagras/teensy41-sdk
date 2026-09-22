#include <teensy/clock.h>
#include <teensy/imxrt.h>
#include <teensy/timer.h>

#include <stddef.h>

#define TIMER_CHANNEL_COUNT 4u
#define TIMER_UNSET_PRIORITY 255u

typedef struct {
  void (*callback)(void);
  uint8_t priority; /* TIMER_UNSET_PRIORITY marks an unused channel */
  uint8_t oneshot;  /* timer_oneshot() channels self-stop after firing */
} timer_state_t;

static timer_state_t timer_states[TIMER_CHANNEL_COUNT];

typedef volatile IMXRT_PIT_CHANNEL_t *timer_channel_regs_t;

static timer_channel_regs_t timer_channel_regs(timer_channel_t channel) {
  return &IMXRT_PIT_CHANNELS[(uint8_t)channel];
}

/* The RT1062 PIT counter runs from ipg_perclk, whose source CCM selects in
 * CSCMR1[PERCLK_CLK_SEL]: 1 = the 24 MHz crystal oscillator, 0 = the IPG
 * clock.  clock_init leaves it on the crystal, so calibrate the conversion
 * against the active mux rather than assuming either rate. */
#define TIMER_PERCLK_OSCILLATOR_HZ 24000000u

static uint32_t timer_pit_hz(void) {
  if ((CCM_CSCMR1 & CCM_CSCMR1_PERCLK_CLK_SEL) != 0u)
    return TIMER_PERCLK_OSCILLATOR_HZ;
  return clock_bus_frequency_hz;
}

/* Converts microseconds to a PIT reload value; the PIT fires once every
 * LDVAL+1 counter clocks.  Rounds up so a period is never shorter than
 * requested.
 * @return TIMER_OK with *reload set, or TIMER_ERROR_RANGE. */
static int timer_reload_from_us(uint32_t microseconds, uint32_t *reload) {
  uint64_t counts;

  if (microseconds == 0u)
    return TIMER_ERROR_RANGE;
  counts =
      ((uint64_t)microseconds * (uint64_t)timer_pit_hz() + 999999u) / 1000000u;
  if (counts > 0xFFFFFFFFull)
    return TIMER_ERROR_RANGE; /* ~179 s maximum at the 24 MHz source */
  *reload = (uint32_t)counts - 1u;
  return TIMER_OK;
}

/* Priority for the shared PIT interrupt: the most urgent (numerically
 * lowest) active channel, or TIMER_UNSET_PRIORITY when none is running. */
static uint8_t timer_top_priority(void) {
  uint8_t top = TIMER_UNSET_PRIORITY;
  uint8_t i;

  for (i = 0; i < TIMER_CHANNEL_COUNT; ++i) {
    if (timer_states[i].callback != NULL && timer_states[i].priority < top)
      top = timer_states[i].priority;
  }
  return top;
}

static void timer_dispatch_isr(void) {
  uint8_t i;

  for (i = 0; i < TIMER_CHANNEL_COUNT; ++i) {
    timer_channel_regs_t regs = timer_channel_regs((timer_channel_t)i);
    void (*callback)(void) = timer_states[i].callback;

    if ((regs->TFLG & PIT_TFLG_TIF) == 0u)
      continue;

    regs->TFLG = PIT_TFLG_TIF; /* W1C so the next period can re-arm */
    if (callback == NULL)
      continue; /* stale flag on an unused channel */

    if (timer_states[i].oneshot != 0u) {
      /* Free the channel before running user code so the callback can
       * re-arm the same channel or anything else immediately. */
      regs->TCTRL = 0;
      timer_states[i].callback = NULL;
      timer_states[i].oneshot = 0;
      timer_states[i].priority = TIMER_UNSET_PRIORITY;
      uint8_t top = timer_top_priority();
      if (top != TIMER_UNSET_PRIORITY)
        NVIC_SET_PRIORITY(IRQ_PIT, top);
    }

    callback();
  }
}

int timer_periodic(timer_channel_t channel, uint32_t period_us,
                   void (*callback)(void), uint8_t priority) {
  timer_channel_regs_t regs;
  uint32_t reload;
  int result;

  if ((uint8_t)channel >= TIMER_CHANNEL_COUNT || callback == NULL)
    return TIMER_ERROR_CHANNEL;

  regs = timer_channel_regs(channel);
  if ((regs->TCTRL & PIT_TCTRL_TEN) != 0u)
    return TIMER_ERROR_BUSY;

  result = timer_reload_from_us(period_us, &reload);
  if (result != TIMER_OK)
    return result;

  /* Program the channel while stopped so a stale pending flag cannot
   * fire the callback spuriously on re-arm. */
  regs->TCTRL = 0;
  regs->TFLG = PIT_TFLG_TIF;
  regs->LDVAL = reload;

  timer_states[channel].callback = callback;
  timer_states[channel].oneshot = 0;
  timer_states[channel].priority = priority;

  NVIC_CLEAR_PENDING(IRQ_PIT);
  _VectorsRam[IRQ_PIT + 16] = timer_dispatch_isr;
  NVIC_ENABLE_IRQ(IRQ_PIT);
  NVIC_SET_PRIORITY(IRQ_PIT, timer_top_priority());

  /* TIE|TEN arms the interrupt and starts the counter. */
  regs->TCTRL = PIT_TCTRL_TIE | PIT_TCTRL_TEN;
  return TIMER_OK;
}

int timer_oneshot(timer_channel_t channel, uint32_t delay_us,
                  void (*callback)(void), uint8_t priority) {
  timer_channel_regs_t regs;
  uint32_t reload;
  int result;

  if ((uint8_t)channel >= TIMER_CHANNEL_COUNT || callback == NULL)
    return TIMER_ERROR_CHANNEL;

  regs = timer_channel_regs(channel);
  if ((regs->TCTRL & PIT_TCTRL_TEN) != 0u)
    return TIMER_ERROR_BUSY;

  result = timer_reload_from_us(delay_us, &reload);
  if (result != TIMER_OK)
    return result;

  regs->TCTRL = 0;
  regs->TFLG = PIT_TFLG_TIF;
  regs->LDVAL = reload;

  timer_states[channel].callback = callback;
  timer_states[channel].oneshot = 1;
  timer_states[channel].priority = priority;

  NVIC_CLEAR_PENDING(IRQ_PIT);
  _VectorsRam[IRQ_PIT + 16] = timer_dispatch_isr;
  NVIC_ENABLE_IRQ(IRQ_PIT);
  NVIC_SET_PRIORITY(IRQ_PIT, timer_top_priority());

  regs->TCTRL = PIT_TCTRL_TIE | PIT_TCTRL_TEN;
  return TIMER_OK;
}

void timer_stop(timer_channel_t channel) {
  timer_channel_regs_t regs;
  uint8_t top;

  if ((uint8_t)channel >= TIMER_CHANNEL_COUNT)
    return;

  regs = timer_channel_regs(channel);
  regs->TCTRL = 0;
  regs->TFLG = PIT_TFLG_TIF;
  timer_states[channel].callback = NULL;
  timer_states[channel].oneshot = 0;
  timer_states[channel].priority = TIMER_UNSET_PRIORITY;

  top = timer_top_priority();
  if (top != TIMER_UNSET_PRIORITY)
    NVIC_SET_PRIORITY(IRQ_PIT, top);
  else
    NVIC_DISABLE_IRQ(IRQ_PIT);
}

bool timer_running(timer_channel_t channel) {
  if ((uint8_t)channel >= TIMER_CHANNEL_COUNT)
    return false;
  return (timer_channel_regs(channel)->TCTRL & PIT_TCTRL_TEN) != 0u;
}
