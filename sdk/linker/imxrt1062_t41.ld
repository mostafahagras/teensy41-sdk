/* Teensy 4.1 memory layout, from the PJRC Teensyduino core. */
MEMORY
{
    ITCM (rx):  ORIGIN = 0x00000000, LENGTH = 512K
    DTCM (rw):  ORIGIN = 0x20000000, LENGTH = 512K
    RAM (rw):   ORIGIN = 0x20200000, LENGTH = 512K
    FLASH (rx): ORIGIN = 0x60000000, LENGTH = 7936K
    ERAM (rw):  ORIGIN = 0x70000000, LENGTH = 32768K
}

ENTRY(ImageVectorTable)

SECTIONS
{
    .text.headers : {
        KEEP(*(.flashconfig))
        FILL(0xFF)
        . = ORIGIN(FLASH) + 0x1000;
        KEEP(*(.ivt))
        KEEP(*(.bootdata))
        . = ALIGN(1024);
    } > FLASH

    .text.code : {
        KEEP(*(.startup))
        *(.flashmem*)
        . = ALIGN(4);
        KEEP(*(.init))
        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        __preinit_array_end = .;
        __init_array_start = .;
        KEEP(*(.init_array))
        __init_array_end = .;
        . = ALIGN(4);
    } > FLASH

    .text.progmem : {
        *(.progmem*)
        . = ALIGN(4);
    } > FLASH

    .text.itcm : {
        . = . + 32;
        *(.fastrun)
        *(.text*)
        . = ALIGN(16);
    } > ITCM AT> FLASH

    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .ARM.extab.text* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > ITCM AT> FLASH

    .data : {
        *(.endpoint_queue)
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*)))
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.data*)))
        KEEP(*(.vectorsram))
    } > DTCM AT> FLASH

    .bss ALIGN(ORIGIN(DTCM) + SIZEOF(.data), 4) : {
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.bss*)))
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(COMMON)))
        . = ALIGN(32);
        . = . + 32;
    } > DTCM

    .bss.dma (NOLOAD) : {
        *(.hab_log)
        *(.dmabuffers)
        . = ALIGN(32);
    } > RAM

    .bss.extram (NOLOAD) : {
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.externalram)))
        . = ALIGN(32);
    } > ERAM

    /* FlexSPI2/PSRAM initialization is not part of the SDK yet.  Fail at
       link time instead of producing an image that accesses uninitialized
       external memory. */
    ASSERT(SIZEOF(.bss.extram) == 0,
           "external RAM variables require FlexSPI2/PSRAM initialization")

    .text.csf : {
        FILL(0xFF)
        . = ALIGN(1024);
        KEEP(*(.csf))
        __text_csf_end = .;
    } > FLASH

    _stext = ADDR(.text.itcm);
    _etext = ADDR(.text.itcm) + SIZEOF(.text.itcm) + SIZEOF(.ARM.exidx);
    _stextload = LOADADDR(.text.itcm);
    _sdata = ADDR(.data);
    _edata = ADDR(.data) + SIZEOF(.data);
    _sdataload = LOADADDR(.data);
    _sbss = ADDR(.bss);
    _ebss = ADDR(.bss) + SIZEOF(.bss);
    _heap_start = ADDR(.bss.dma) + SIZEOF(.bss.dma);
    _heap_end = ORIGIN(RAM) + LENGTH(RAM);
    _itcm_block_count = (SIZEOF(.text.itcm) + SIZEOF(.ARM.exidx) + 0x7FFF) >> 15;
    _flexram_bank_config = 0xAAAAAAAA | ((1 << (_itcm_block_count * 2)) - 1);
    _estack = ORIGIN(DTCM) + ((16 - _itcm_block_count) << 15);
    _flashimagelen = __text_csf_end - ORIGIN(FLASH);
    _teensy_model_identifier = 0x25;

    .debug_info 0 : { *(.debug_info) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_line 0 : { *(.debug_line) }
    .debug_frame 0 : { *(.debug_frame) }
    .debug_str 0 : { *(.debug_str) }
    .debug_loc 0 : { *(.debug_loc) }
}
