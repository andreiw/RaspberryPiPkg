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

#include <Uefi.h>
#include <HypDxe.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Chipset/AArch64.h>
#include <Protocol/DebugSupport.h>
#include <Utils.h>

#define HCR_RW_64 BIT31
#define SPSR_D    BIT9
#define SPSR_EL1  0x4
#define SPSR_ELx  0x1
#define ReadSysReg(var, reg) asm volatile("mrs %0, " #reg : "=r" (var))
#define WriteSysReg(reg, val) asm volatile("msr " #reg ", %0"  : : "r" (val))
#define ISB() asm volatile("isb");

#define SPSR_2_EL(spsr) (_X(spsr, 2, 3))
#define SPSR_2_BITNESS(spsr) (_X(spsr, 4, 4) ? 32 : 64)
#define SPSR_2_SPSEL(spsr) (_X(spsr, 0, 0) ? 1 : 0)

typedef struct UEFI_EL2_STATE
{
  UINT64 Hcr;
  UINT64 Cptr;
  UINT64 Cnthctl;
  UINT64 Cntvoff;
  UINT64 Vpidr;
  UINT64 Vmpidr;
  UINT64 Vttbr;
  UINT64 Vtcr;
  UINT64 Sctlr;
  UINT64 Actlr;
  UINT64 Tcr;
  UINT64 Ttbr0;
  UINT64 Vbar;
  UINT64 Mair;
  UINT64 Spsel;
} UEFI_EL2_STATE;


STATIC VOID
CaptureEL2State(OUT UEFI_EL2_STATE *State)
{
  ReadSysReg(State->Hcr, hcr_el2);
  ReadSysReg(State->Cptr, cptr_el2);
  ReadSysReg(State->Cnthctl, cnthctl_el2);
  ReadSysReg(State->Cntvoff, cntvoff_el2);
  ReadSysReg(State->Vpidr, vpidr_el2);
  ReadSysReg(State->Vmpidr, vmpidr_el2);
  ReadSysReg(State->Vttbr, vttbr_el2);
  ReadSysReg(State->Vtcr, vtcr_el2);
  ReadSysReg(State->Sctlr, sctlr_el2);
  ReadSysReg(State->Actlr, actlr_el2);
  ReadSysReg(State->Ttbr0, ttbr0_el2);
  ReadSysReg(State->Tcr, tcr_el2);
  ReadSysReg(State->Vbar, vbar_el2);
  ReadSysReg(State->Mair, mair_el2);
  ReadSysReg(State->Spsel, spsel);
}


STATIC EFI_STATUS
HypModeInit(IN  UEFI_EL2_STATE *State,
            OUT EFI_PHYSICAL_ADDRESS *ExceptionStackTop)
{
  VOID *Stack;
  UINTN StackSize;

  /*
    HCR_EL2         0x8000002 (TGE | SWIO)
    CPTR_EL2        0x33FF    (nothing)
    CNTHCTL_EL2     0x3       (not trapped)
    CNTVOFF_EL2     0x0
    VPIDR_EL2       0x410FD034
    VMPIDR_EL2      0x80000000
    VTTBR_EL2       0x0
    VTCR_EL2        0x80000110 (48-bit, inner WBWA, outer non-cacheable)
    SCTLR_EL2       0x30C5183D (I, C, M)
    ACTLR_EL2       0x0
    TTBR0_EL2       0x3B3F8000
    TCR_EL2         0x80803520 (32-bit, inner WBWA, outer WBWA,
                                inner shareable, 4K, 32-bit physical)
    VBAR_EL2        0x37203000
    MAIR_EL2        0xFFBB4400
    SPSEL           0x1
  */

  DEBUG((EFI_D_INFO, "HCR_EL2    \t0x%lx\n", State->Hcr));
  DEBUG((EFI_D_INFO, "CPTR_EL2   \t0x%lx\n", State->Cptr));
  DEBUG((EFI_D_INFO, "CNTHCTL_EL2\t0x%lx\n", State->Cnthctl));
  DEBUG((EFI_D_INFO, "CNTVOFF_EL2\t0x%lx\n", State->Cntvoff));
  DEBUG((EFI_D_INFO, "VPIDR_EL2  \t0x%lx\n", State->Vpidr));
  DEBUG((EFI_D_INFO, "VMPIDR_EL2 \t0x%lx\n", State->Vmpidr));
  DEBUG((EFI_D_INFO, "VTTBR_EL2  \t0x%lx\n", State->Vttbr));
  DEBUG((EFI_D_INFO, "VTCR_EL2   \t0x%lx\n", State->Vtcr));
  DEBUG((EFI_D_INFO, "SCTLR_EL2  \t0x%lx\n", State->Sctlr));
  DEBUG((EFI_D_INFO, "ACTLR_EL2  \t0x%lx\n", State->Actlr));
  DEBUG((EFI_D_INFO, "TTBR0_EL2  \t0x%lx\n", State->Ttbr0));
  DEBUG((EFI_D_INFO, "TCR_EL2    \t0x%lx\n", State->Tcr));
  DEBUG((EFI_D_INFO, "VBAR_EL2   \t0x%lx\n", State->Vbar));
  DEBUG((EFI_D_INFO, "MAIR_EL2   \t0x%lx\n", State->Mair));
  DEBUG((EFI_D_INFO, "SPSEL      \t0x%lx\n", State->Spsel));

  StackSize = PcdGet32(PcdCPUCorePrimaryStackSize);
  Stack = AllocateRuntimePages(EFI_SIZE_TO_PAGES(StackSize));
  if (Stack == 0) {
    DEBUG((EFI_D_ERROR, "No memory for stack\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  DEBUG((EFI_D_INFO, "%u of EL2 stack at %p\n", StackSize, Stack));

  /* EL1 is 64-bit. */
  WriteSysReg(hcr_el2, HCR_RW_64);
  *ExceptionStackTop = ((EFI_PHYSICAL_ADDRESS) Stack) + StackSize;
  WriteSysReg(vbar_el2, &ExceptionHandlersStart);

  return EFI_SUCCESS;
}


STATIC VOID
HypSwitchToEL1(IN  UEFI_EL2_STATE *State,
               IN  EFI_PHYSICAL_ADDRESS ExceptionStack)
{

  UINT64 tcr_el1;
  UINT64 sctlr_el1;
  UINT64 sp = 0;

  DEBUG((EFI_D_INFO, "Switching to EL1\n"));

  sctlr_el1 =
    I(X(State->Sctlr, 0, 2), 0, 2)     | // C, A, M
    I(1, 12, 12)                | // RES1
    I(X(State->Sctlr, 12, 12), 12, 12) | // I
    I(1, 20, 20)                | // RES1
    I(0x3, 22, 23)              | // RES1
    I(0x3, 28, 29);               // RES1
  WriteSysReg(sctlr_el1, sctlr_el1);

  WriteSysReg(ttbr0_el1, State->Ttbr0);
  WriteSysReg(ttbr1_el1, 0);

  tcr_el1 =
    I(X(State->Tcr, 0, 5), 0, 5)     | // T0SZ
    I(X(State->Tcr, 8, 9), 8, 9)     | // IRGN0
    I(X(State->Tcr, 10, 11), 10, 11) | // ORNG0
    I(X(State->Tcr, 12, 13), 12, 13) | // SH0
    I(X(State->Tcr, 14, 15), 14, 15) | // TG0
    I(1, 23, 23)                     | // EPD1
    I(X(State->Tcr, 16, 18), 32, 34) | // PS -> IPS
    I(X(State->Tcr, 20, 20), 37, 37);  // TBI -> TBI0
  WriteSysReg(tcr_el1, tcr_el1);
  WriteSysReg(vbar_el1, State->Vbar);
  WriteSysReg(mair_el1, State->Mair);
  WriteSysReg(elr_el2, &&done);
  WriteSysReg(spsr_el2, SPSR_D | SPSR_A | SPSR_I |
              SPSR_F | SPSR_EL1 | SPSR_ELx);
  ISB();

  asm volatile("tlbi alle1\n\t"
               "dsb sy\n\t");
  ISB();

  asm volatile("mov %0, sp\n\t"
               "mov sp, %1\n\t"
               "msr sp_el1, %0\n\t"
               "eret\n\t" : "+r"(sp)
               : "r"(ExceptionStack));
 done:
  DEBUG((EFI_D_INFO, "Switched to EL1\n"));
}


VOID
HypExceptionHandler (
  IN     EFI_EXCEPTION_TYPE ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT_AARCH64 *SystemContext
)
{
  if (SPSR_2_BITNESS(SystemContext->SPSR) == 64) {
    if (SPSR_2_EL(SystemContext->SPSR) == 2) {
      DEBUG((EFI_D_ERROR, "Unexpected exception in EL2!\n"));
      while(1);
    }

    DEBUG((EFI_D_ERROR, "Hello from EL%u, EC 0x%x ISS 0x%x\n",
           SPSR_2_EL(SystemContext->SPSR),
           AARCH64_ESR_EC(SystemContext->ESR),
           AARCH64_ESR_ISS(SystemContext->ESR)));
  }
}


EFI_STATUS
EFIAPI
HypInitialize(
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  UINT32 DoEL1;

  if (ArmReadCurrentEL () != AARCH64_EL2) {
    return EFI_UNSUPPORTED;
  }

  DoEL1 = PcdGet32 (PcdBootInEL1);
  if (DoEL1) {
    UINT64 DAIF;
    UEFI_EL2_STATE State;
    EFI_PHYSICAL_ADDRESS ExceptionStack;

    ReadSysReg(DAIF, daif);
    ArmDisableAllExceptions();

    CaptureEL2State(&State);
    Status = HypModeInit(&State, &ExceptionStack);
    if (Status == EFI_SUCCESS) {
      HypSwitchToEL1(&State, ExceptionStack);
    }

    WriteSysReg(daif, DAIF);

    asm volatile("hvc #0x1337");
  }

  /*
   * Update regardless to create NV variable. The
   * config VFR won't work otherwise, failing to save.
   */
  PcdSet32 (PcdBootInEL1, DoEL1);

  Status = InstallHiiPages();
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "Couldn't install HypDxe configuration pages: %r\n",
           Status));
  }

  return EFI_SUCCESS;
}

