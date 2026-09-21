#include <stdint.h>

#include <teensy/cache.h>
#include <teensy/clock.h>
#include <teensy/imxrt.h>
#include <teensy/time.h>

extern uint32_t _estack;
extern uint32_t _stext;
extern uint32_t _etext;
extern uint32_t _stextload;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sdataload;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _flexram_bank_config;

int main(void);

__attribute__((used, aligned(1024), section(".vectorsram"))) void (
        *volatile _VectorsRam[NVIC_NUM_INTERRUPTS + 16])(void);

__attribute__((section(".startup"))) static void
copy_words(uint32_t *destination, const uint32_t *source, uint32_t *end) {
  while (destination < end) {
    *destination++ = *source++;
  }
}

__attribute__((section(".startup"))) static void
clear_words(uint32_t *destination, uint32_t *end) {
  while (destination < end) {
    *destination++ = 0;
  }
}

__attribute__((noreturn)) static void unused_interrupt_vector(void) {
  for (;;) {
    __asm volatile("wfi");
  }
}

__attribute__((section(".startup"))) static void initialize_vectors(void) {
  uint32_t i;

  for (i = 0; i < NVIC_NUM_INTERRUPTS + 16; ++i) {
    _VectorsRam[i] = unused_interrupt_vector;
  }
  for (i = 0; i < NVIC_NUM_INTERRUPTS; ++i) {
    NVIC_SET_PRIORITY(i, 128);
  }
  SCB_VTOR = (uint32_t)_VectorsRam;
  __asm volatile("dsb\nisb" ::: "memory");
}

__attribute__((noreturn, noinline, used, section(".startup"))) static void
reset_handler(void) {
  /* Match the known-good Teensyduino power and PLL PFD setup.  The PFD
   * registers are writable before ITCM/DTCM initialization because this
   * function executes from flash. */
  PMU_MISC0_SET = PMU_MISC0_REFTOP_SELFBIASOFF;
  CCM_ANALOG_PFD_528 = 0x2018101Bu; /* 352, 594, 396, 297 MHz */
  CCM_ANALOG_PFD_480 = 0x13110D0Cu; /* 720, 664, 508, 454 MHz */

  copy_words(&_stext, &_stextload, &_etext);
  copy_words(&_sdata, &_sdataload, &_edata);
  clear_words(&_sbss, &_ebss);

  /*
   * The whole SDK is built with -mfloat-abi=hard and -mfpu=fpv5-d16.
   * Grant full access to CP10 and CP11 before any compiled C code outside
   * this startup section can execute a floating-point instruction.
   */
  SCB_CPACR |= 0x00F00000u;
  __asm volatile("dsb\nisb" ::: "memory");

  initialize_vectors();

  /* Make configurable faults independently diagnosable instead of
   * escalating them directly to HardFault. */
  SCB_SHCSR |=
      SCB_SHCSR_MEMFAULTENA | SCB_SHCSR_BUSFAULTENA | SCB_SHCSR_USGFAULTENA;

  // Route the fast GPIO6-GPIO9 aliases used by the Teensy 4.1 pin map.
  IOMUXC_GPR_GPR26 = 0xFFFFFFFFu;
  IOMUXC_GPR_GPR27 = 0xFFFFFFFFu;
  IOMUXC_GPR_GPR28 = 0xFFFFFFFFu;
  IOMUXC_GPR_GPR29 = 0xFFFFFFFFu;

  cache_init();
  clock_init(F_CPU);

  /* The boot ROM uses PIT while loading the image.  Do not expose that
   * inherited timer state to the application. */
  CCM_CCGR1 |= CCM_CCGR1_PIT(CCM_CCGR_ON);
  PIT_MCR = 0;
  PIT_TCTRL0 = 0;
  PIT_TCTRL1 = 0;
  PIT_TCTRL2 = 0;
  PIT_TCTRL3 = 0;

  time_init();
  SCB_SCR &= ~(SCB_SCR_SLEEPDEEP | SCB_SCR_SLEEPONEXIT);
  __enable_irq();

  (void)main();
  for (;;) {
    __asm volatile("wfi");
  }
}

__attribute__((naked, used, section(".startup"))) void ResetHandler(void) {
  __asm volatile("ldr r0, =_flexram_bank_config\n"
                 "ldr r1, =0x400AC000\n"
                 "str r0, [r1, #68]\n"
                 "ldr r0, =0x00200007\n"
                 "str r0, [r1, #64]\n"
                 "ldr r0, =0x00AA0000\n"
                 "str r0, [r1, #56]\n"
                 "dsb\n"
                 "ldr r0, =_estack\n"
                 "mov sp, r0\n"
                 "bl reset_handler\n"
                 "1: wfi\n"
                 "b 1b\n");
}
