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

#include "HypDxe.h"
#include "ArmDefs.h"
#include <Library/ArmSmcLib.h>


VOID
HypSMCProcess(
  IN  EFI_SYSTEM_CONTEXT_AARCH64 *SystemContext
  )
{
  if (ESR_2_ISS(SystemContext->ESR) == 0x0) {
    switch (SystemContext->X0) {
    case PSCI_CPU_OFF:
      SystemContext->X0 = UN(PSCI_RETURN_STATUS_DENIED);
      return;
    case PSCI_CPU_ON_64:
      SystemContext->X0 = UN(HypSMPOn(SystemContext->X1,
                                      SystemContext->X2,
                                      SystemContext->X3));
      return;
    }
  }

  DEBUG((EFI_D_VERBOSE, "0x%lx: Forwarding SMC(%u) %x %x %x %x\n",
         SystemContext->ELR, ESR_2_ISS(SystemContext->ESR),
         SystemContext->X0, SystemContext->X1,
         SystemContext->X2, SystemContext->X3));
  ArmCallSmc((ARM_SMC_ARGS *) &(SystemContext->X0));
}
