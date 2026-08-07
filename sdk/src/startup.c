#include <stdint.h>

#include <teensy/imxrt.h>
#include <teensy/clock.h>
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

__attribute__((used, aligned(1024), section(".vectorsram")))
void (* volatile _VectorsRam[NVIC_NUM_INTERRUPTS + 16])(void);

__attribute__((section(".startup")))
static void copy_words(uint32_t *destination, const uint32_t *source,
                       uint32_t *end)
{
    while (destination < end) {
        *destination++ = *source++;
    }
}

__attribute__((section(".startup")))
static void clear_words(uint32_t *destination, uint32_t *end)
{
    while (destination < end) {
        *destination++ = 0;
    }
}

__attribute__((noreturn))
static void unused_interrupt_vector(void)
{
    for (;;) {
        __asm volatile("wfi");
    }
}

__attribute__((section(".startup")))
static void initialize_vectors(void)
{
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

__attribute__((noreturn, noinline, used, section(".startup")))
static void reset_handler(void)
{
    IOMUXC_GPR_GPR17 = (uint32_t)&_flexram_bank_config;
    IOMUXC_GPR_GPR16 = 0x00200007u;
    IOMUXC_GPR_GPR14 = 0x00AA0000u;
    __asm volatile("dsb" ::: "memory");

    copy_words(&_stext, &_stextload, &_etext);
    copy_words(&_sdata, &_sdataload, &_edata);
    clear_words(&_sbss, &_ebss);
    initialize_vectors();

    // Route the fast GPIO6-GPIO9 aliases used by the Teensy 4.1 pin map.
    IOMUXC_GPR_GPR26 = 0xFFFFFFFFu;
    IOMUXC_GPR_GPR27 = 0xFFFFFFFFu;
    IOMUXC_GPR_GPR28 = 0xFFFFFFFFu;
    IOMUXC_GPR_GPR29 = 0xFFFFFFFFu;

    clock_init(F_CPU);
    time_init();

    (void)main();
    for (;;) {
        __asm volatile("wfi");
    }
}

__attribute__((naked, used, section(".startup")))
void ResetHandler(void)
{
    __asm volatile(
        "ldr r0, =_estack\n"
        "mov sp, r0\n"
        "bl reset_handler\n"
        "1: wfi\n"
        "b 1b\n");
}
