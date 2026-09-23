#include <stdint.h>

#include <teensy/cache.h>
#include <teensy/imxrt.h>

extern uint32_t _ebss;

#define NOEXEC SCB_MPU_RASR_XN
#define READONLY SCB_MPU_RASR_AP(7)
#define READWRITE SCB_MPU_RASR_AP(3)
#define NOACCESS SCB_MPU_RASR_AP(0)
#define MEM_CACHE_WT (SCB_MPU_RASR_TEX(0) | SCB_MPU_RASR_C)
#define MEM_CACHE_WB (SCB_MPU_RASR_TEX(0) | SCB_MPU_RASR_C | SCB_MPU_RASR_B)
#define MEM_CACHE_WBWA (SCB_MPU_RASR_TEX(1) | SCB_MPU_RASR_C | SCB_MPU_RASR_B)
#define MEM_NOCACHE SCB_MPU_RASR_TEX(1)
#define DEV_NOCACHE SCB_MPU_RASR_TEX(2)
#define SIZE_32B (SCB_MPU_RASR_SIZE(4) | SCB_MPU_RASR_ENABLE)
#define SIZE_128K (SCB_MPU_RASR_SIZE(16) | SCB_MPU_RASR_ENABLE)
#define SIZE_512K (SCB_MPU_RASR_SIZE(18) | SCB_MPU_RASR_ENABLE)
#define SIZE_1M (SCB_MPU_RASR_SIZE(19) | SCB_MPU_RASR_ENABLE)
#define SIZE_16M (SCB_MPU_RASR_SIZE(23) | SCB_MPU_RASR_ENABLE)
#define SIZE_32M (SCB_MPU_RASR_SIZE(24) | SCB_MPU_RASR_ENABLE)
#define SIZE_1G (SCB_MPU_RASR_SIZE(29) | SCB_MPU_RASR_ENABLE)
#define REGION(n) (SCB_MPU_RBAR_REGION(n) | SCB_MPU_RBAR_VALID)

/* MPU region entries: RASR, then RBAR with the absolute base. */
typedef struct {
  uint32_t rbar;
  uint32_t rasr;
} cache_region_t;

void cache_init(void) {
  SCB_MPU_CTRL = 0;

  { /* Regions 0..9: constants from the previous straight-line code. */
    static const cache_region_t regions[] = {
        {REGION(0), NOACCESS | NOEXEC | SCB_MPU_RASR_TEX(0) |
                        SCB_MPU_RASR_SIZE(31) | SCB_MPU_RASR_ENABLE},
        {0x00000000u | REGION(1), MEM_NOCACHE | READONLY | SIZE_512K},
        {0x00000000u | REGION(2), DEV_NOCACHE | NOACCESS | SIZE_32B},
        {0x00200000u | REGION(3), MEM_CACHE_WT | READONLY | SIZE_128K},
        {0x20000000u | REGION(4), MEM_NOCACHE | READWRITE | NOEXEC | SIZE_512K},
        {0x20200000u | REGION(6),
         MEM_CACHE_WBWA | READWRITE | NOEXEC | SIZE_1M},
        {0x40000000u | REGION(7),
         DEV_NOCACHE | READWRITE | NOEXEC |
             (SCB_MPU_RASR_SIZE(25) | SCB_MPU_RASR_ENABLE)},
        {0x60000000u | REGION(8), MEM_CACHE_WBWA | READONLY | SIZE_16M},
        {0x70000000u | REGION(9),
         MEM_CACHE_WBWA | READWRITE | NOEXEC | SIZE_32M},
        {0x80000000u | REGION(11),
         MEM_CACHE_WBWA | READWRITE | NOEXEC | SIZE_1G},
    };

    for (uint32_t i = 0; i < sizeof(regions) / sizeof(regions[0]); ++i) {
      SCB_MPU_RBAR = regions[i].rbar;
      SCB_MPU_RASR = regions[i].rasr;
    }

    /* Region 10 depends on a linker symbol: the stack-overflow trap just
     * past the BSS end. */
    SCB_MPU_RBAR = (uint32_t)&_ebss | REGION(5);
    SCB_MPU_RASR = NOACCESS | NOEXEC | SCB_MPU_RASR_TEX(0) | SIZE_32B;
  }

  __asm volatile("nop\nnop\nnop\nnop\nnop");
  SCB_MPU_CTRL = SCB_MPU_CTRL_ENABLE;
  __asm volatile("dsb\nisb" ::: "memory");
  SCB_CACHE_ICIALLU = 0;
  __asm volatile("dsb\nisb" ::: "memory");
  SCB_CCR |= SCB_CCR_IC | SCB_CCR_DC;
  __asm volatile("dsb\nisb" ::: "memory");
}
