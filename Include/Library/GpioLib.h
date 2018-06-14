/** @file
 *
 *  GPIO manipulation.
 *
 *  Copyright (c) 2018, Andrei Warkentin <andrey.warkentin@gmail.com>
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

#ifndef __GPIO_LIB__
#define __GPIO_LIB__

#include <IndustryStandard/Bcm2837Gpio.h>

VOID
GpioPinFuncSet(
  IN  UINTN Pin,
  IN  UINTN Function
  );

UINTN
GpioPinFuncGet(
  IN  UINTN Pin
  );

#endif /* __GPIO_LIB__ */
