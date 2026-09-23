/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2019 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdint.h>

#include <teensy/boottime.h>
#include <teensy/clock.h>
#include <teensy/imxrt.h>

/* Fast-startup notes:  reset_handler raises the DCDC target and waits for
 * STS_DC_OK before calling in, so the voltages are already correct and no
 * settle wait happens here; the F_CPU ladder folds at -Os, leaving only
 * the volatile register writes and the CCM handshake busy-waits.
 *
 * The DCDC regulator values moved to reset_handler so their settle time
 * does not gate the clock switch: the target must be stable before the
 * switch, but the register write can be fired well before the switch. */

/* Runtime upper bound (matches PJRC's overclock ladder; the datasheet
 * mentions how the DOWN ladder behaves differently). */
#define OVERCLOCK_STEPSIZE 28000000u
#define OVERCLOCK_MAX_VOLT 1575u

// Exported clock rates (overloadable by watchdogs and clock-speed knobs).
volatile uint32_t clock_cpu_frequency_hz = 396000000u;
volatile uint32_t clock_bus_frequency_hz = 132000000u;
volatile uint32_t clock_uart_frequency_hz = 24000000u;

uint32_t clock_init(uint32_t frequency) {
  uint32_t cbcdr;
  uint32_t cbcmr;

  /* LPUART peripherals use the 24 MHz crystal clock. */
  CCM_CSCMR1 =
      (CCM_CSCMR1 & ~CCM_CSCMR1_PERCLK_PODF(0x3F)) | CCM_CSCMR1_PERCLK_CLK_SEL;
  CCM_CSCDR1 =
      (CCM_CSCDR1 & ~CCM_CSCDR1_UART_CLK_PODF(0x3F)) | CCM_CSCDR1_UART_CLK_SEL;
  CCM_CSCDR2 = (CCM_CSCDR2 & ~CCM_CSCDR2_LPI2C_CLK_PODF(0x3F)) |
               CCM_CSCDR2_LPI2C_CLK_SEL;

  boottime_cycles_clock_entry = ARM_DWT_CYCCNT;

  /* Periph stage: switch to the running-and-locked USB PLL as a stable
   * intermediate, exactly the way the original core's PERIPH_CLK2 dance
   * does it. */
  /* Alternate-source dance, semantics identical to the core's
   * set_arm_clock: skip when already on PERIPH_CLK2; otherwise pick the
   * USB PLL (120 MHz intermediate) if it is running, else the 24 MHz
   * crystal, so the CPU keeps running while the ARM PLL rebuilds. */
  cbcdr = CCM_CBCDR;
  cbcmr = CCM_CBCMR;
  if ((cbcdr & CCM_CBCDR_PERIPH_CLK_SEL) == 0u) {
    const uint32_t need1s =
        CCM_ANALOG_PLL_USB1_ENABLE | CCM_ANALOG_PLL_USB1_POWER |
        CCM_ANALOG_PLL_USB1_LOCK | CCM_ANALOG_PLL_USB1_EN_USB_CLKS;
    uint32_t sel;
    uint32_t div;
    if ((CCM_ANALOG_PLL_USB1 & need1s) == need1s) {
      sel = 0u;
      div = 3u; // 480/4 = 120 MHz, so IPG is ok even at IPG_PODF=0
    } else {
      sel = 1u;
      div = 0u;
    }
    if ((cbcdr & CCM_CBCDR_PERIPH_CLK2_PODF_MASK) !=
        CCM_CBCDR_PERIPH_CLK2_PODF(div)) {
      cbcdr &= ~CCM_CBCDR_PERIPH_CLK2_PODF_MASK;
      cbcdr |= CCM_CBCDR_PERIPH_CLK2_PODF(div);
      CCM_CBCDR = cbcdr;
    }
    if ((cbcmr & CCM_CBCMR_PERIPH_CLK2_SEL_MASK) !=
        CCM_CBCMR_PERIPH_CLK2_SEL(sel)) {
      cbcmr &= ~CCM_CBCMR_PERIPH_CLK2_SEL_MASK;
      cbcmr |= CCM_CBCMR_PERIPH_CLK2_SEL(sel);
      CCM_CBCMR = cbcmr;
      while (CCM_CDHIPR & CCM_CDHIPR_PERIPH2_CLK_SEL_BUSY)
        ; // wait
    }
    cbcdr |= CCM_CBCDR_PERIPH_CLK_SEL;
    CCM_CBCDR = cbcdr;
    while (CCM_CDHIPR & CCM_CDHIPR_PERIPH_CLK_SEL_BUSY)
      ; // wait
  }

  // ARM PLL: DIV_SELECT = 54-108 maps 648-1296 MHz in 12 MHz steps;
  // with F_CPU = 600 MHz the core frequency folds to mult 100 (= 1.2 GHz
  // locked by the ROM): plain register writes remain.
#if F_CPU == 600000000u
  const uint32_t div_arm = 2u;
  const uint32_t div_ahb = 1u;
  const uint32_t mult = 100u; /* 1200 MHz = 100 * 12 MHz */
#else
#error "the caret block only folds for F_CPU 600 MHz in this round"
#endif

  const uint32_t arm_pll_mask =
      CCM_ANALOG_PLL_ARM_LOCK | CCM_ANALOG_PLL_ARM_BYPASS |
      CCM_ANALOG_PLL_ARM_ENABLE | CCM_ANALOG_PLL_ARM_POWERDOWN |
      CCM_ANALOG_PLL_ARM_DIV_SELECT_MASK;
  if ((CCM_ANALOG_PLL_ARM & arm_pll_mask) !=
      (CCM_ANALOG_PLL_ARM_LOCK | CCM_ANALOG_PLL_ARM_ENABLE |
       CCM_ANALOG_PLL_ARM_DIV_SELECT(mult))) {
    CCM_ANALOG_PLL_ARM = CCM_ANALOG_PLL_ARM_POWERDOWN;
    // TODO: delay needed?
    CCM_ANALOG_PLL_ARM =
        CCM_ANALOG_PLL_ARM_ENABLE | CCM_ANALOG_PLL_ARM_DIV_SELECT(mult);
    while (!(CCM_ANALOG_PLL_ARM & CCM_ANALOG_PLL_ARM_LOCK))
      ; // wait for lock
  }
  boottime_cycles_pll_done = ARM_DWT_CYCCNT;

  if ((CCM_CACRR & CCM_CACRR_ARM_PODF_MASK) != (div_arm - 1)) {
    CCM_CACRR = CCM_CACRR_ARM_PODF(div_arm - 1);
    while (CCM_CDHIPR & CCM_CDHIPR_ARM_PODF_BUSY)
      ; // wait
  }

  if ((cbcdr & CCM_CBCDR_AHB_PODF_MASK) != (div_ahb - 1)) {
    cbcdr &= ~CCM_CBCDR_AHB_PODF_MASK;
    cbcdr |= CCM_CBCDR_AHB_PODF(div_ahb - 1);
    CCM_CBCDR = cbcdr;
    while (CCM_CDHIPR & CCM_CDHIPR_AHB_PODF_BUSY)
      ; // wait
  }

#if F_CPU == 600000000u
  const uint32_t div_ipg = 4u; /* IPG = 150 MHz */
#else
  uint32_t div_ipg = (frequency + 149999999u) / 150000000u;
  if (div_ipg > 4u)
    div_ipg = 4u;
#endif
  if ((cbcdr & CCM_CBCDR_IPG_PODF_MASK) != (CCM_CBCDR_IPG_PODF(div_ipg - 1))) {
    cbcdr &= ~CCM_CBCDR_IPG_PODF_MASK;
    cbcdr |= CCM_CBCDR_IPG_PODF(div_ipg - 1);
    // TODO: how to safely change IPG_PODF ??
    CCM_CBCDR = cbcdr;
  }

  CCM_CBCDR &= ~CCM_CBCDR_PERIPH_CLK_SEL;
  while (CCM_CDHIPR & CCM_CDHIPR_PERIPH_CLK_SEL_BUSY)
    ; // wait

  clock_cpu_frequency_hz = frequency;
  clock_bus_frequency_hz = frequency / div_ipg;
  clock_uart_frequency_hz = 24000000u;

  /* the fired target equals the ladder target; no settle wait remains */
  boottime_cycles_dcdc_done = ARM_DWT_CYCCNT;

  return frequency;
}
