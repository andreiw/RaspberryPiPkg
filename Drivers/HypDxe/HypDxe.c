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
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/Bcm2836.h>
#include <Library/ArmSmcLib.h>
#include <Utils.h>

#define ReadSysReg(var, reg) asm volatile("mrs %0, " #reg : "=r" (var))
#define WriteSysReg(reg, val) asm volatile("msr " #reg ", %0"  : : "r" (val))
#define ISB() asm volatile("isb");
#define DSB_ISH() asm volatile("dsb ish");

#define SPSR_2_EL(spsr) (X((spsr), 2, 3))
#define SPSR_2_BITNESS(spsr) (X((spsr), 4, 4) ? 32 : 64)
#define SPSR_2_SPSEL(spsr) (X((spsr), 0, 0) ? 1 : 0)

#define ESR_EC_HVC64 0x16
#define ESR_EC_SMC64 0x17
#define ESR_2_IL(x)  (X((x), 25, 25))
#define ESR_2_EC(x)  (X((x), 26, 31))
#define ESR_2_ISS(x) (X((x), 0, 24))

#define HCR_RW_64 BIT31
#define HCR_TSC   BIT19
#define SPSR_D    BIT9
#define SPSR_EL1  0x4
#define SPSR_ELx  0x1


/*
 * I share UEFI's MAIR.
 */
#define PTE_ATTR_MEM TT_ATTR_INDX_MEMORY_WRITE_BACK
#define PTE_ATTR_DEV TT_ATTR_INDX_DEVICE_MEMORY
#define PTE_RW       I(0x1, 6, 7)
#define PTE_SH_INNER I(0x3, 8, 9)
#define PTE_AF       I(0x1, 10, 10)

#define MBYTES_2_BYTES(x) ((x) * 1024 * 1024)
#define PTE_TYPE_TAB      0x3
#define PTE_TYPE_BLOCK    0x1
#define PTE_TYPE_BLOCK_L3 0x3
#define PTE_2_TYPE(x)     ((x) & 0x3)
/*
 * Assumes 4K granularity.
 */
#define PTE_2_TAB(x)      ((VOID *)M((x), 47, 12))
#define VA_2_PL1_IX(x)    X((x), 30, 38)
#define VA_2_PL2_IX(x)    X((x), 21, 29)

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
HypBuildPT(IN  UEFI_EL2_STATE *State)
{
  UINT64 *PL1;
  EFI_PHYSICAL_ADDRESS A = 0;
  EFI_PHYSICAL_ADDRESS E = BCM2836_SOC_REGISTERS +
    BCM2836_SOC_REGISTER_LENGTH;
  /*
   * This is not a general purpose page table
   * builder.
   *
   * T0SZ assumed 32-bit.
   * Granule 4K.
   */
  ASSERT (X(State->Tcr, 0, 5) == (64 - 32));
  ASSERT (X(State->Tcr, 14, 15) == 0x0);

  PL1 = (VOID *) AllocateRuntimePages(1);
  if (PL1 == NULL) {
    DEBUG((EFI_D_ERROR, "Couldn't alloc L1 table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem(PL1, EFI_PAGE_SIZE);

  while (A < E) {
    /*
     * Each PL2 covers 1GB, each entry being 2MB.
     */
    UINT64 *PL2;

    UINTN ix1 = VA_2_PL1_IX(A);
    UINTN ix2 = VA_2_PL2_IX(A);

    PL2 = PTE_2_TAB(PL1[ix1]);
    if (PTE_2_TYPE(PL1[ix1]) != PTE_TYPE_TAB) {
      PL2 = (VOID *) AllocateRuntimePages(1);
      if (PL2 == NULL) {
        DEBUG((EFI_D_ERROR, "Couldn't alloc L2 table for %u\n",
               ix1));
        return EFI_OUT_OF_RESOURCES;
      }

      ZeroMem(PL2, EFI_PAGE_SIZE);
      PL1[ix1] = ((UINTN) PL2) | PTE_TYPE_TAB;
    }

    PL2[ix2] = PTE_TYPE_BLOCK | PTE_RW | PTE_SH_INNER |
      PTE_AF | A;
    if (A >= BCM2836_SOC_REGISTERS) {
      PL2[ix2] |= PTE_ATTR_DEV;
    } else {
      PL2[ix2] |= PTE_ATTR_MEM;
    }

    A += MBYTES_2_BYTES(2);
  }

  DEBUG((EFI_D_INFO, "Setting page table root to 0x%lx\n",
         (UINT64) PL1));

  DSB_ISH();
  WriteSysReg(ttbr0_el2, PL1);
  ISB();

  asm volatile("tlbi alle2");
  DSB_ISH();
  ISB();

  DEBUG((EFI_D_INFO, "It lives!\n"));

  return EFI_SUCCESS;
}


STATIC EFI_STATUS
HypModeInit(IN  UEFI_EL2_STATE *State,
            OUT EFI_PHYSICAL_ADDRESS *ExceptionStackTop)
{
  VOID *Stack;
  UINTN StackSize;
  EFI_STATUS Status;

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

  /* EL1 is 64-bit, and we trap SMC calls. */
  WriteSysReg(hcr_el2, HCR_RW_64 | HCR_TSC);
  *ExceptionStackTop = ((EFI_PHYSICAL_ADDRESS) Stack) + StackSize;
  WriteSysReg(vbar_el2, &ExceptionHandlersStart);

  /*
   * Initialize the page tables. We need EL2 ones (we cannot
   * reuse UEFI's, because those are allocated as boot service data,
   * and thus are gone after OS boots. We also need second stage
   * tables so we can control the view of the OS into the physical
   * address space.
   */
  Status = HypBuildPT(State);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

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
HypExceptionFatal (
  IN     EFI_EXCEPTION_TYPE ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT_AARCH64 *SystemContext
)
{
  if (SPSR_2_BITNESS(SystemContext->SPSR) == 32) {
    DEBUG((EFI_D_ERROR, "Unexpected exception from AArch32!\n"));
    goto done;
  }

  if (SPSR_2_EL(SystemContext->SPSR) == 2) {
    DEBUG((EFI_D_ERROR, "Unexpected exception in EL2!\n"));
    goto done;
  }

done:
  DEBUG((EFI_D_ERROR, "FAR = 0x%lx\n",
         SystemContext->FAR));
  while(1);
}


VOID
HypExceptionHandler (
  IN     EFI_EXCEPTION_TYPE ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT_AARCH64 *SystemContext
)
{
  if (SPSR_2_BITNESS(SystemContext->SPSR) == 32 ||
      SPSR_2_EL(SystemContext->SPSR) == 2) {
    HypExceptionFatal(ExceptionType, SystemContext);
  }

  switch (ESR_2_EC(SystemContext->ESR)) {
  case ESR_EC_HVC64:
    DEBUG((EFI_D_ERROR, "Hello from EL%u PC 0x%016lx SP 0x%016lx\n",
           SPSR_2_EL(SystemContext->SPSR),
           SystemContext->ELR, SystemContext->SP));
    break;
  case ESR_EC_SMC64: {
    DEBUG((EFI_D_INFO, "0x%lx: Forwarding SMC(%u) %x %x %x %x\n",
           SystemContext->ELR, ESR_2_ISS(SystemContext->ESR),
           SystemContext->X0, SystemContext->X1,
           SystemContext->X2, SystemContext->X3));
    ArmCallSmc((ARM_SMC_ARGS *) &(SystemContext->X0));
    SystemContext->ELR += 4;
    break;
  }

  default:
    HypExceptionFatal(ExceptionType, SystemContext);
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

  return EFI_SUCCESS;
}

