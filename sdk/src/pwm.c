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

#include <teensy/clock.h>
#include <teensy/gpio.h>
#include <teensy/imxrt.h>
#define TEENSY_PWM_IMPLEMENTATION
#include <teensy/pwm.h>

uint8_t pwm_resolution_bits = 8;

#if defined(__IMXRT1062__)

#define PWM_PIN_ENTRY(number, type, module, channel, muxval)                   \
  [number] = {type, module, channel, muxval},

const pwm_pin_info_t pwm_pin_info[TEENSY_GPIO_PIN_COUNT] = {
    TEENSY_PWM_PIN_MAP(PWM_PIN_ENTRY)};

#undef PWM_PIN_ENTRY

// Known usage of FlexPWM and QuadTimers
// -------------------------------------
//   FlexPWM1_0    PWM pin 1, 36(T4.0), 37(T4.0), 44(T4.1), 45(T4.1)
//   FlexPWM1_1    PWM pin 0, 34(T4.0), 35(T4.0), 42(T4.1), 43(T4.1)
//   FlexPWM1_2    PWM pin 24, 38(T4.0), 39(T4.0), 46(T4.1), 47(T4.1)
//   FlexPWM1_3    PWM pin 7, 8, 25
//   FlexPWM2_0    PWM pin 4, 33
//   FlexPWM2_1    PWM pin 5, Tlc5940 library
//   FlexPWM2_2    PWM pin 6, 9
//   FlexPWM2_3    PWM pin 36(T4.1), 37(T4.1)
//   FlexPWM3_0    PWM pin 53(T4.1)
//   FlexPWM3_1    PWM pin 28, 29
//   FlexPWM3_2
//   FlexPWM3_3    PWM pin 51(T4.1)
//   FlexPWM4_0    PWM pin 22
//   FlexPWM4_1    PWM pin 23
//   FlexPWM4_2    PWM pin 2, 3, Tlc5940 library
//   FlexPWM4_3
//   QuadTimer1_0  PWM pin 10
//   QuadTimer1_1  PWM pin 12
//   QuadTimer1_2  PWM pin 11
//   QuadTimer1_3
//   QuadTimer2_0  PWM pin 13
//   QuadTimer2_1
//   QuadTimer2_2
//   QuadTimer2_3
//   QuadTimer3_0  PWM pin 19
//   QuadTimer3_1  PWM pin 18
//   QuadTimer3_2  PWM pin 14
//   QuadTimer3_3  PWM pin 15
//   QuadTimer4_0  OctoWS2811, ADC library
//   QuadTimer4_1  OctoWS2811
//   QuadTimer4_2  OctoWS2811
//   QuadTimer4_3  AudioInputAnalog, ADC library

#endif // __IMXRT1062__

static int pwm_write_info(const pwm_pin_info_t *info, const gpio_pin_t *gpio,
                          uint32_t val) {
  if (info == NULL || gpio == NULL)
    return -1;
  if (info->type == 1) {
    IMXRT_FLEXPWM_t *flexpwm;
    switch ((info->module >> 4) & 3) {
    case 0:
      flexpwm = &IMXRT_FLEXPWM1;
      break;
    case 1:
      flexpwm = &IMXRT_FLEXPWM2;
      break;
    case 2:
      flexpwm = &IMXRT_FLEXPWM3;
      break;
    default:
      flexpwm = &IMXRT_FLEXPWM4;
    }
    (void)gpio;
    return pwm_write_flex(flexpwm, info->module & 3u, info->channel, val);
  }
  if (info->type == 2) {
    IMXRT_TMR_t *qtimer;
    switch ((info->module >> 4) & 3) {
    case 0:
      qtimer = &IMXRT_TMR1;
      break;
    case 1:
      qtimer = &IMXRT_TMR2;
      break;
    case 2:
      qtimer = &IMXRT_TMR3;
      break;
    default:
      qtimer = &IMXRT_TMR4;
    }
    (void)gpio;
    return pwm_write_quad(qtimer, info->module & 3u, val);
  }
  return -1;
}

/* Register-block math: reads live hardware state (VAL1/LOAD) and the
 * global duty resolution; one shared copy serves both the typed (const
 * pin) path and the runtime fallback. */
int pwm_write_flex(IMXRT_FLEXPWM_t *p, uint8_t submodule, uint8_t channel,
                   uint16_t value) {
  uint16_t mask = (uint16_t)(1u << submodule);
  uint32_t modulo = p->SM[submodule].VAL1;
  uint32_t cval = ((uint32_t)value * (modulo + 1u)) >> pwm_resolution_bits;

  if (cval > modulo)
    cval = modulo;
  p->MCTRL |= FLEXPWM_MCTRL_CLDOK(mask);
  switch (channel) {
  case 0:
    p->SM[submodule].VAL0 = modulo - cval;
    p->OUTEN |= FLEXPWM_OUTEN_PWMX_EN(mask);
    break;
  case 1:
    p->SM[submodule].VAL3 = cval;
    p->OUTEN |= FLEXPWM_OUTEN_PWMA_EN(mask);
    break;
  case 2:
    p->SM[submodule].VAL5 = cval;
    p->OUTEN |= FLEXPWM_OUTEN_PWMB_EN(mask);
    break;
  default:
    return -1;
  }
  p->MCTRL |= FLEXPWM_MCTRL_LDOK(mask);
  return 0;
}

int pwm_write_quad(IMXRT_TMR_t *p, uint8_t submodule, uint16_t value) {
  uint32_t modulo = 65537u - p->CH[submodule].LOAD + p->CH[submodule].CMPLD1;
  uint32_t high = ((uint32_t)value * (modulo - 1u)) >> pwm_resolution_bits;
  uint32_t low;

  if (high >= modulo - 1u)
    high = modulo - 2u;
  low = modulo - high;
  p->CH[submodule].LOAD = (uint16_t)(65537u - low);
  p->CH[submodule].CMPLD1 = (uint16_t)high;
  return 0;
}

int pwm_frequency_flex(IMXRT_FLEXPWM_t *p, uint8_t submodule, uint8_t channel,
                       float frequency_hz) {
  uint16_t mask = (uint16_t)(1u << submodule);
  uint32_t olddiv = p->SM[submodule].VAL1;
  uint32_t newdiv =
      (uint32_t)((float)clock_bus_frequency_hz / frequency_hz + 0.5f);
  uint32_t prescale = 0;

  (void)channel;
  while (newdiv > 65535u && prescale < 7u) {
    newdiv = newdiv >> 1;
    prescale = prescale + 1u;
  }
  if (newdiv > 65535u)
    newdiv = 65535u;
  else if (newdiv < 2u)
    newdiv = 2u;
  p->MCTRL |= FLEXPWM_MCTRL_CLDOK(mask);
  p->SM[submodule].CTRL = FLEXPWM_SMCTRL_FULL | FLEXPWM_SMCTRL_PRSC(prescale);
  p->SM[submodule].VAL1 = newdiv - 1u;
  p->SM[submodule].VAL0 = (p->SM[submodule].VAL0 * newdiv) / olddiv;
  p->SM[submodule].VAL3 = (p->SM[submodule].VAL3 * newdiv) / olddiv;
  p->SM[submodule].VAL5 = (p->SM[submodule].VAL5 * newdiv) / olddiv;
  p->MCTRL |= FLEXPWM_MCTRL_LDOK(mask);
  return 0;
}

int pwm_frequency_quad(IMXRT_TMR_t *p, uint8_t submodule, float frequency_hz) {
  uint32_t newdiv =
      (uint32_t)((float)clock_bus_frequency_hz / frequency_hz + 0.5f);
  uint32_t prescale = 0;
  uint32_t oldhigh;
  uint32_t oldlow;
  uint32_t high;
  uint32_t low;

  while (newdiv > 65534u && prescale < 7u) {
    newdiv = newdiv >> 1;
    prescale = prescale + 1u;
  }
  if (newdiv > 65534u)
    newdiv = 65534u;
  else if (newdiv < 2u)
    newdiv = 2u;
  oldhigh = p->CH[submodule].CMPLD1;
  oldlow = 65537u - p->CH[submodule].LOAD;
  high = (oldhigh * newdiv) / (oldhigh + oldlow);
  low = newdiv - high;
  p->CH[submodule].LOAD = (uint16_t)(65537u - low);
  p->CH[submodule].CMPLD1 = (uint16_t)high;
  p->CH[submodule].CTRL = TMR_CTRL_CM(1) | TMR_CTRL_PCS(8 + prescale) |
                          TMR_CTRL_LENGTH | TMR_CTRL_OUTMODE(6);
  return 0;
}

int pwm_write_impl(uint8_t pin, uint32_t value) {
  if (pin >= TEENSY_GPIO_PIN_COUNT)
    return -1;
  return pwm_write_info(&pwm_pin_info[pin], gpio_pin_runtime(pin), value);
}

static int pwm_set_frequency_info(const pwm_pin_info_t *info,
                                  const gpio_pin_t *gpio, float frequency) {
  if (info == NULL || gpio == NULL || frequency <= 0.0f)
    return -1;
  if (info->type == 1) {
    IMXRT_FLEXPWM_t *flexpwm;
    switch ((info->module >> 4) & 3) {
    case 0:
      flexpwm = &IMXRT_FLEXPWM1;
      break;
    case 1:
      flexpwm = &IMXRT_FLEXPWM2;
      break;
    case 2:
      flexpwm = &IMXRT_FLEXPWM3;
      break;
    default:
      flexpwm = &IMXRT_FLEXPWM4;
    }
    (void)gpio;
    return pwm_frequency_flex(flexpwm, info->module & 3u, info->channel,
                              frequency);
  }
  if (info->type == 2) {
    IMXRT_TMR_t *qtimer;
    switch ((info->module >> 4) & 3) {
    case 0:
      qtimer = &IMXRT_TMR1;
      break;
    case 1:
      qtimer = &IMXRT_TMR2;
      break;
    case 2:
      qtimer = &IMXRT_TMR3;
      break;
    default:
      qtimer = &IMXRT_TMR4;
    }
    (void)gpio;
    return pwm_frequency_quad(qtimer, info->module & 3u, frequency);
  }
  return -1;
}

int pwm_set_frequency_impl(uint8_t pin, float frequency_hz) {
  if (pin >= TEENSY_GPIO_PIN_COUNT)
    return -1;
  return pwm_set_frequency_info(&pwm_pin_info[pin], gpio_pin_runtime(pin),
                                frequency_hz);
}

static void flexpwm_init(IMXRT_FLEXPWM_t *p) {
  int i;

  p->FCTRL0 = FLEXPWM_FCTRL0_FLVL(15); // logic high = fault
  p->FSTS0 = 0x000F;                   // clear fault status
  p->FFILT0 = 0;
  p->MCTRL |= FLEXPWM_MCTRL_CLDOK(15);
  for (i = 0; i < 4; i++) {
    p->SM[i].CTRL2 =
        FLEXPWM_SMCTRL2_INDEP | FLEXPWM_SMCTRL2_WAITEN | FLEXPWM_SMCTRL2_DBGEN;
    p->SM[i].CTRL = FLEXPWM_SMCTRL_FULL;
    p->SM[i].OCTRL = 0;
    p->SM[i].DTCNT0 = 0;
    p->SM[i].INIT = 0;
    p->SM[i].VAL0 = 0;
    p->SM[i].VAL1 = 33464;
    p->SM[i].VAL2 = 0;
    p->SM[i].VAL3 = 0;
    p->SM[i].VAL4 = 0;
    p->SM[i].VAL5 = 0;
  }
  p->MCTRL |= FLEXPWM_MCTRL_LDOK(15);
  p->MCTRL |= FLEXPWM_MCTRL_RUN(15);
}

static void quadtimer_init(IMXRT_TMR_t *p) {
  int i;

  for (i = 0; i < 4; i++) {
    p->CH[i].CTRL = 0; // stop timer
    p->CH[i].CNTR = 0;
    p->CH[i].SCTRL =
        TMR_SCTRL_OEN | TMR_SCTRL_OPS | TMR_SCTRL_VAL | TMR_SCTRL_FORCE;
    p->CH[i].CSCTRL = TMR_CSCTRL_CL1(1) | TMR_CSCTRL_ALT_LOAD;
    // COMP must be less than LOAD - otherwise output is always low
    p->CH[i].LOAD = 24000; // low time  (65537 - x) -
    p->CH[i].COMP1 = 0;    // high time (0 = always low, max = LOAD-1)
    p->CH[i].CMPLD1 = 0;
    p->CH[i].CTRL = TMR_CTRL_CM(1) | TMR_CTRL_PCS(8) | TMR_CTRL_LENGTH |
                    TMR_CTRL_OUTMODE(6);
  }
}

void pwm_init(void) {
  CCM_CCGR4 |= CCM_CCGR4_PWM1(CCM_CCGR_ON) | CCM_CCGR4_PWM2(CCM_CCGR_ON) |
               CCM_CCGR4_PWM3(CCM_CCGR_ON) | CCM_CCGR4_PWM4(CCM_CCGR_ON);
  CCM_CCGR6 |= CCM_CCGR6_QTIMER1(CCM_CCGR_ON) | CCM_CCGR6_QTIMER2(CCM_CCGR_ON) |
               CCM_CCGR6_QTIMER3(CCM_CCGR_ON) | CCM_CCGR6_QTIMER4(CCM_CCGR_ON);
  flexpwm_init(&IMXRT_FLEXPWM1);
  flexpwm_init(&IMXRT_FLEXPWM2);
  flexpwm_init(&IMXRT_FLEXPWM3);
  flexpwm_init(&IMXRT_FLEXPWM4);
  quadtimer_init(&IMXRT_TMR1);
  quadtimer_init(&IMXRT_TMR2);
  quadtimer_init(&IMXRT_TMR3);
}

static void xbar_connect(unsigned int input, unsigned int output) {
  if (input >= 88)
    return;
  if (output >= 132)
    return;
#if 1
  volatile uint16_t *xbar = &XBARA1_SEL0 + (output / 2);
  uint16_t val = *xbar;
  if (!(output & 1)) {
    val = (val & 0xFF00) | input;
  } else {
    val = (val & 0x00FF) | (input << 8);
  }
  *xbar = val;
#else
  // does not work, seems 8 bit access is not allowed
  volatile uint8_t *xbar = (volatile uint8_t *)XBARA1_SEL0;
  xbar[output] = input;
#endif
}

uint32_t pwm_set_resolution_impl(uint32_t bits) {
  uint32_t prior;
  if (bits < 1) {
    bits = 1;
  } else if (bits > 16) {
    bits = 16;
  }
  prior = pwm_resolution_bits;
  pwm_resolution_bits = bits;
  return prior;
}
