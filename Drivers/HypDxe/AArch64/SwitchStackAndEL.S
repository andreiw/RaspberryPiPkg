/** @file
 *
 *  Copyright (c), 2018, Andrei Warkentin <andrey.warkentin@gmail.com>
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

#include <AsmMacroIoLibV8.h>

GCC_ASM_EXPORT(SwitchStackAndEL)
        
.text

ASM_PFX(SwitchStackAndEL):
  mov      x1, sp
  mov      sp, x0
  msr      sp_el1, x1
  msr      elr_el2, x30
  eret
