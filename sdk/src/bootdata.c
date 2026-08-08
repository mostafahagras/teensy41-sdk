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
 * selection among a list of target devices, then similar target devices
 * manufactured by PJRC.COM must be included in the list of target devices
 * and selectable in the same manner.
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

extern void ResetHandler(void);
extern unsigned long _flashimagelen;

__attribute__((section(".bootdata"), used))
const uint32_t BootData[3] = {0x60000000u, (uint32_t)&_flashimagelen, 0};

__attribute__((section(".csf"), used)) const uint32_t hab_csf[768];

__attribute__((section(".ivt"), used))
const uint32_t ImageVectorTable[8] = {0x432000D1u,
                                      (uint32_t)&ResetHandler,
                                      0,
                                      0,
                                      (uint32_t)BootData,
                                      (uint32_t)ImageVectorTable,
                                      (uint32_t)hab_csf,
                                      0};

__attribute__((section(".flashconfig"),
               used)) uint32_t FlexSPI_NOR_Config[128] = {
    /* Common FlexSPI configuration block, words 0-31. */
    0x42464346u, 0x56010000u, 0, 0x00020101u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0x01060401u, 0, 0, 0x00800000u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    /* LUT, words 32-111. */
    0x0A1804EBu, 0x32041EFFu, 0x00002601u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    /* Serial NOR configuration block, words 112-127. */
    256, 4096, 1, 0, 0x00010000u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
