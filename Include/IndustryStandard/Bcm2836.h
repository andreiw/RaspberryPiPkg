/*
 * Copyright (c), 2017, Andrei Warkentin <andrey.warkentin@gmail.com>
 * Copyright (c) 2016, Linaro Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Both "core" and SoC perpherals (1M each).
 */
#define BCM2836_SOC_REGISTERS                               0x3f000000
#define BCM2836_SOC_REGISTER_LENGTH                         0x02000000

/* Synopsis DW2/DWC USB 2.0 OTG. */
#define BCM2836_USB_DW2_BASE_ADDRESS                        0x3f980000

/*
 * Offset between the CPU's view and the VC's view of system memory.
 */
#define BCM2836_DMA_DEVICE_OFFSET                           0xc0000000

/* watchdog constants */
#define BCM2836_WDOG_BASE_ADDRESS                           0x3f100000
#define BCM2836_WDOG_PASSWORD                               0x5a000000
#define BCM2836_WDOG_RSTC_OFFSET                            0x0000001c
#define BCM2836_WDOG_WDOG_OFFSET                            0x00000024
#define BCM2836_WDOG_RSTC_WRCFG_MASK                        0x00000030
#define BCM2836_WDOG_RSTC_WRCFG_FULL_RESET                  0x00000020

/* mailbox interface constants */
#define BCM2836_MBOX_BASE_ADDRESS                           0x3f00b880
#define BCM2836_MBOX_READ_OFFSET                            0x00000000
#define BCM2836_MBOX_STATUS_OFFSET                          0x00000018
#define BCM2836_MBOX_CONFIG_OFFSET                          0x0000001c
#define BCM2836_MBOX_WRITE_OFFSET                           0x00000020

#define BCM2836_MBOX_STATUS_FULL                            0x1f
#define BCM2836_MBOX_STATUS_EMPTY                           0x1e

#define BCM2836_MBOX_NUM_CHANNELS                           16

/* interrupt controller constants */
#define BCM2836_INTC_TIMER_CONTROL_OFFSET                   0x00000040
#define BCM2836_INTC_TIMER_PENDING_OFFSET                   0x00000060
