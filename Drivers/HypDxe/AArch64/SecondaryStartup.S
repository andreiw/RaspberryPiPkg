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

GCC_ASM_EXPORT(SecondaryStartup)

.text

ASM_PFX(SecondaryStartup):
  //
  // x0 = &CPU_ON_STATE
  // x1 = &CPU_ON_STATE.EL2
  //
  // This must stay in sync with CPU_ON_STATE
  // and CAPTURE_EL2_STATE.
  //
  add      x1, x0, #0x18

  //
  // Enable paging.
  //
  ldp      x2, x3, [x1], #0x10
  msr      vbar_el2, x2
  msr      mair_el2, x3
  ldp      x2, x3, [x1], #0x10
  msr      tcr_el2, x2
  msr      ttbr0_el2, x3
  ldp      x2, x3, [x1], #0x10
  msr      actlr_el2, x2
  msr      sctlr_el2, x3
  isb
  tlbi     alle2
  dsb      ish
  isb

  //
  // Load stack and go the rest of initialization.
  //
  msr      spsel, 1
  ldr      x1, [x0, #0]
  mov      sp, x1
  b        HypSMPContinueStartup
