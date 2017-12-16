/*
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

#define RPI_FW_MBOX_CHANNEL                                 0x00000008

#define RPI_FW_RESP_SUCCESS                                 0x80000000
#define RPI_FW_RESP_FAILURE                                 0x80000001

#define RPI_FW_VALUE_SIZE_RESPONSE_MASK                     BIT31

#define RPI_FW_GET_REVISION                                 0x00000001
#define RPI_FW_GET_BOARD_MODEL                              0x00010001
#define RPI_FW_GET_BOARD_REVISION                           0x00010002
#define RPI_FW_GET_MAC_ADDRESS                              0x00010003
#define RPI_FW_GET_BOARD_SERIAL                             0x00010004
#define RPI_FW_GET_ARM_MEMSIZE                              0x00010005

#define RPI_FW_SET_POWER_STATE                              0x00028001

#define RPI_FW_POWER_STATE_SDHCI                            0x00000000
#define RPI_FW_POWER_STATE_UART0                            0x00000001
#define RPI_FW_POWER_STATE_UART1                            0x00000002
#define RPI_FW_POWER_STATE_USB_HCD                          0x00000003
#define RPI_FW_POWER_STATE_I2C0                             0x00000004
#define RPI_FW_POWER_STATE_I2C1                             0x00000005
#define RPI_FW_POWER_STATE_I2C2                             0x00000006
#define RPI_FW_POWER_STATE_SPI                              0x00000007
#define RPI_FW_POWER_STATE_CCP2TX                           0x00000008

#define RPI_FW_GET_CLOCK_RATE                               0x00030002
#define RPI_FW_GET_MAX_CLOCK_RATE                           0x00030004
#define RPI_FW_GET_MIN_CLOCK_RATE                           0x00030007

#define RPI_FW_GET_FB_GEOMETRY                              0x00040003
#define RPI_FW_GET_FB_LINELENGTH                            0x00040008
#define RPI_FW_GET_FB_COLOR_DEPTH                           0x00040005
#define RPI_FW_GET_FB_REGION                                0x00040001
#define RPI_FW_SET_GPIO                                     0x00038041
#define RPI_FW_GET_COMMAND_LINE                             0x00050001

#define RPI_FW_SET_FB_PGEOM                                 0x00048003
#define RPI_FW_SET_FB_VGEOM                                 0x00048004
#define RPI_FW_SET_FB_DEPTH                                 0x00048005
#define RPI_FW_ALLOC_FB                                     0x00040001
#define RPI_FW_FREE_FB                                      0x00048001

#define RPI_FW_POWER_STATE_ENABLE                           BIT0
#define RPI_FW_POWER_STATE_WAIT                             BIT1

#define RPI_FW_CLOCK_RATE_EMMC                              0x000000001
#define RPI_FW_CLOCK_RATE_UART                              0x000000002
#define RPI_FW_CLOCK_RATE_ARM                               0x000000003
#define RPI_FW_CLOCK_RATE_CORE                              0x000000004
#define RPI_FW_CLOCK_RATE_V3D                               0x000000005
#define RPI_FW_CLOCK_RATE_H264                              0x000000006
#define RPI_FW_CLOCK_RATE_ISP                               0x000000007
#define RPI_FW_CLOCK_RATE_SDRAM                             0x000000008
#define RPI_FW_CLOCK_RATE_PIXEL                             0x000000009
#define RPI_FW_CLOCK_RATE_PWM                               0x00000000a

#define RPI_FB_MBOX_CHANNEL                                 0x1
