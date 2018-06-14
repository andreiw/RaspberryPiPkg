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

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/GpioLib.h>
#include <IndustryStandard/Bcm2836.h>
#include <Utils.h>

STATIC VOID
GpioFSELModify(
  IN  UINTN RegIndex,
  IN  UINT32 ModifyMask,
  IN  UINT32 FunctionMask
  )
{
  UINT32 Val;
  EFI_PHYSICAL_ADDRESS Reg = RegIndex * sizeof(UINT32) + GPIO_GPFSEL0;

  ASSERT(Reg <= GPIO_GPFSEL5);
  ASSERT((~ModifyMask & FunctionMask) == 0);

  Val = MmioRead32(Reg);
  Val &= ~ModifyMask;
  Val |= FunctionMask;
  MmioWrite32(Reg, Val);
}

VOID
GpioPinFuncSet(
  IN  UINTN Pin,
  IN  UINTN Function
  )
{
  UINTN RegIndex = Pin / 10;
  UINTN SelIndex = Pin % 10;
  UINT32 ModifyMask;
  UINT32 FunctionMask;

  ASSERT(Pin < GPIO_PINS);
  ASSERT(Function <= GPIO_FSEL_MASK);

  ModifyMask = GPIO_FSEL_MASK << (SelIndex * GPIO_FSEL_BITS_PER_PIN);
  FunctionMask = Function << (SelIndex * GPIO_FSEL_BITS_PER_PIN);
  GpioFSELModify(RegIndex, ModifyMask, FunctionMask);
}

UINTN
GpioPinFuncGet(
  IN  UINTN Pin
  )
{
  UINT32 Val;
  UINTN RegIndex = Pin / 10;
  UINTN SelIndex = Pin % 10;
  EFI_PHYSICAL_ADDRESS Reg = RegIndex * sizeof(UINT32) + GPIO_GPFSEL0;

  ASSERT(Pin < GPIO_PINS);

  Val = MmioRead32(Reg);
  Val >>= SelIndex * GPIO_FSEL_BITS_PER_PIN;
  Val &= GPIO_FSEL_MASK;
  return Val;
}
