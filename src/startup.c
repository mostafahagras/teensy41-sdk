#include <stdint.h>

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

#define IOMUXC_GPR_GPR14 (*(volatile uint32_t *)0x400AC038u)
#define IOMUXC_GPR_GPR16 (*(volatile uint32_t *)0x400AC040u)
#define IOMUXC_GPR_GPR17 (*(volatile uint32_t *)0x400AC044u)

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
