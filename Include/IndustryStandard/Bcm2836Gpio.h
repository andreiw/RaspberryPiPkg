/** @file
*
*  Copyright (c), Microsoft Corporation. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __BCM2836GPIO_H__
#define __BCM2836GPIO_H__

#define GPIO_BASE_ADDRESS  (BCM2836_SOC_REGISTERS + 0x00200000)

#define GPIO_GPFSEL0       (GPIO_BASE_ADDRESS + 0x00)
#define GPIO_GPFSEL1       (GPIO_BASE_ADDRESS + 0x04)
#define GPIO_GPFSEL2       (GPIO_BASE_ADDRESS + 0x08)
#define GPIO_GPFSEL3       (GPIO_BASE_ADDRESS + 0x0C)
#define GPIO_GPFSEL4       (GPIO_BASE_ADDRESS + 0x10)
#define GPIO_GPFSEL5       (GPIO_BASE_ADDRESS + 0x14)

#define GPIO_GPCLR0        (GPIO_BASE_ADDRESS + 0x28)
#define GPIO_GPCLR1        (GPIO_BASE_ADDRESS + 0x2C)

#define GPIO_GPSET0        (GPIO_BASE_ADDRESS + 0x1C)
#define GPIO_GPSET1        (GPIO_BASE_ADDRESS + 0x20)

#define GPIO_FSEL_INPUT    0x0
#define GPIO_FSEL_OUTPUT   0x1
#define GPIO_FSEL_ALT0     0x4
#define GPIO_FSEL_ALT1     0x5
#define GPIO_FSEL_ALT2     0x6
#define GPIO_FSEL_ALT3     0x7
#define GPIO_FSEL_ALT4     0x3
#define GPIO_FSEL_ALT5     0x2

#define GPIO_FSEL_PINS_PER_REGISTER 10
#define GPIO_FSEL_BITS_PER_PIN      3
#define GPIO_FSEL_MASK              ((1 << GPIO_FSEL_BITS_PER_PIN) - 1)

#endif // __BCM2836GPIO_H__

