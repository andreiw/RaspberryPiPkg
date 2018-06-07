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
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>

typedef struct {
  UINT64 SPx;
  UINT64 EL1PC;
  UINT64 EL1Arg;
  CAPTURED_EL2_STATE EL2;
} CPU_ON_STATE;

STATIC BOOLEAN mCpuOnLock = SL_UNLOCKED;


VOID
HypSMPContinueStartup(
  IN  CPU_ON_STATE *State
  )
{
  UINT64 Mpidr;
  UINT64 EL1Arg;

  /*
   * EL2 state.
   */
  WriteSysReg(cnthctl_el2, State->EL2.Cnthctl);
  WriteSysReg(cntvoff_el2, State->EL2.Cntvoff);

  /*
   * State controlling EL1.
   */
  WriteSysReg(vpidr_el2, State->EL2.Vpidr);
  ReadSysReg(Mpidr, mpidr_el1);
  WriteSysReg(vmpidr_el2, Mpidr);
  WriteSysReg(vtcr_el2, State->EL2.Vtcr);
  WriteSysReg(vttbr_el2, State->EL2.Vttbr);
  WriteSysReg(cptr_el2, State->EL2.Cptr);
  WriteSysReg(hcr_el2, State->EL2.Hcr);
  WriteSysReg(mdcr_el2, State->EL2.Mdcr);
  TLBI_S12();

  /*
   * EL1 state proper.
   */
  WriteSysReg(sctlr_el1, SCTLR_EL1_RES1);
  WriteSysReg(spsr_el2, SPSR_D | SPSR_A | SPSR_I |
              SPSR_F | SPSR_EL1 | SPSR_ELx);
  WriteSysReg(elr_el2, State->EL1PC);
  EL1Arg = State->EL1Arg;

  State = NULL;
  SUnlock(&mCpuOnLock);

  asm volatile("mov x0, %0\n\t"
               "eret" : : "r" (EL1Arg));
}


PSCI_Status
HypSMPOn(
  IN  UINT64 MPIDR,
  IN  GPA    EL1PC,
  IN  UINT64 EL1Arg
  )
{
  VOID *Stack;
  UINTN StackSize;
  CPU_ON_STATE *State;
  ARM_SMC_ARGS PsciArgs;

  HLOG((HLOG_VERBOSE, "CPU_ON for core %lx 0x%lx(0x%lx)\n",
        MPIDR, EL1PC, EL1Arg));

  /*
   * One page should be more than enough ;-).
   */
  StackSize = EFI_PAGE_SIZE;
  Stack = HypMemAlloc(EFI_SIZE_TO_PAGES(StackSize));
  if (Stack == NULL) {
    HLOG((HLOG_ERROR, "No memory for stack\n"));
    return PSCI_RETURN_INTERNAL_FAILURE;
  }

  HLOG((HLOG_VERBOSE, "%u of EL2 stack at %p\n",
        StackSize, Stack));

  State = Stack;
  State->SPx = UN(Stack) + StackSize;
  State->EL1PC = EL1PC;
  State->EL1Arg = EL1Arg;
  CaptureEL2State(&State->EL2);

  WriteBackDataCacheRange(Stack, StackSize);

  SLock(&mCpuOnLock);

  PsciArgs.Arg0 = PSCI_CPU_ON_64;
  PsciArgs.Arg1 = MPIDR;
  PsciArgs.Arg2 = UN(&SecondaryStartup);
  PsciArgs.Arg3 = UN(State);
  ArmCallSmc(&PsciArgs);

  if (PsciArgs.Arg0 != PSCI_RETURN_STATUS_SUCCESS) {
    __atomic_clear(&mCpuOnLock, __ATOMIC_RELEASE);
    return PsciArgs.Arg0;
  }

  /*
   * Wait for other core to wake up and initialize,
   * dropping the lock.
   */
  SLock(&mCpuOnLock);
  SUnlock(&mCpuOnLock);

  return PSCI_RETURN_STATUS_SUCCESS;
}

