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
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/Bcm2836.h>


VOID
CaptureEL2State(
  OUT CAPTURED_EL2_STATE *State
)
{
  ReadSysReg(State->Hcr, hcr_el2);
  ReadSysReg(State->Cptr, cptr_el2);
  ReadSysReg(State->Cnthctl, cnthctl_el2);
  ReadSysReg(State->Cntvoff, cntvoff_el2);
  ReadSysReg(State->Vpidr, vpidr_el2);
  ReadSysReg(State->Vttbr, vttbr_el2);
  ReadSysReg(State->Vtcr, vtcr_el2);
  ReadSysReg(State->Sctlr, sctlr_el2);
  ReadSysReg(State->Actlr, actlr_el2);
  ReadSysReg(State->Ttbr0, ttbr0_el2);
  ReadSysReg(State->Tcr, tcr_el2);
  ReadSysReg(State->Vbar, vbar_el2);
  ReadSysReg(State->Mair, mair_el2);
}


STATIC EFI_STATUS
HypBuildS2PT(IN  CAPTURED_EL2_STATE *State)
{
  UINT64 Vtcr;
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

  PL1 = (VOID *) HypMemAlloc(1);
  if (PL1 == NULL) {
    DEBUG((EFI_D_ERROR, "Couldn't alloc S2 L1 table\n"));
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
      PL2 = (VOID *) HypMemAlloc(1);
      if (PL2 == NULL) {
        DEBUG((EFI_D_ERROR, "Couldn't alloc S2 L2 table for %u\n",
               ix1));
        return EFI_OUT_OF_RESOURCES;
      }

      ZeroMem(PL2, EFI_PAGE_SIZE);
      PL1[ix1] = ((UINTN) PL2) | PTE_TYPE_TAB;
    }

    if (HypMemIsHyp2M(A)) {
      UINT64 *PL3;
      UINTN ix3 = VA_2_PL3_IX(A);

      /*
       * HypDxe range, need to break into 4K pages
       * and make sure HypDxe is RO.
       */
      PL3 = PTE_2_TAB(PL2[ix2]);
      if (PTE_2_TYPE(PL2[ix2]) != PTE_TYPE_TAB) {
        PL3 = (VOID *) HypMemAlloc(1);
        if (PL3 == NULL) {
          DEBUG((EFI_D_ERROR, "Couldn't alloc S2 L3 table for %u\n",
                 ix2));
          return EFI_OUT_OF_RESOURCES;
        }

        ZeroMem(PL3, EFI_PAGE_SIZE);
        PL2[ix2] = ((UINTN) PL3) | PTE_TYPE_TAB;
      }

      PL3[ix3] = PTE_TYPE_BLOCK_L3 | PTE_SH_INNER  |
        PTE_AF | A | PTE_S2_ATTR_MEM;
      if (HypMemIsHypAddr(A)) {
        PL3[ix3] |= PTE_S2_RO;
      } else {
        PL3[ix3] |= PTE_S2_RW;
      }

      A += SIZE_4KB;
    } else {
      PL2[ix2] = PTE_TYPE_BLOCK | PTE_S2_RW |
        PTE_SH_INNER | PTE_AF | A;
      if (A >= BCM2836_SOC_REGISTERS) {
        PL2[ix2] |= PTE_S2_ATTR_DEV;
      } else {
        PL2[ix2] |= PTE_S2_ATTR_MEM;
      }

      A += SIZE_2MB;
    }
  }

  Vtcr =
    I(X(State->Tcr, 0, 5), 0, 5)     | // T0SZ
    I(1, 6, 7)                       | // SL0 == L1
    I(X(State->Tcr, 8, 9), 8, 9)     | // IRGN0
    I(X(State->Tcr, 10, 11), 10, 11) | // ORNG0
    I(X(State->Tcr, 12, 13), 12, 13) | // SH0
    I(X(State->Tcr, 14, 15), 14, 15) | // TG0
    I(X(State->Tcr, 16, 18), 16, 18) | // PS -> IPS
    I(1, 31, 31);                      // RESV1
  DEBUG((EFI_D_INFO, "Setting S2 page table root to 0x%lx\n",
         (UINT64) PL1));

  DSB_ISH();
  WriteSysReg(vtcr_el2, Vtcr);
  WriteSysReg(vttbr_el2, PL1);
  TLBI_S12();

  return EFI_SUCCESS;
}


STATIC EFI_STATUS
HypBuildPT(IN  CAPTURED_EL2_STATE *State)
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

  PL1 = (VOID *) HypMemAlloc(1);
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
      PL2 = (VOID *) HypMemAlloc(1);
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

    A += SIZE_2MB;
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
HypModeInit(IN  CAPTURED_EL2_STATE *State,
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
    VTTBR_EL2       0x0
    VTCR_EL2        0x80000110 (48-bit, inner WBWA, outer non-cacheable)
    SCTLR_EL2       0x30C5183D (I, C, M)
    ACTLR_EL2       0x0
    TTBR0_EL2       0x3B3F8000
    TCR_EL2         0x80803520 (32-bit, inner WBWA, outer WBWA,
                                inner shareable, 4K, 32-bit physical)
    VBAR_EL2        0x37203000
    MAIR_EL2        0xFFBB4400
  */

  DEBUG((EFI_D_INFO, "HCR_EL2    \t0x%lx\n", State->Hcr));
  DEBUG((EFI_D_INFO, "CPTR_EL2   \t0x%lx\n", State->Cptr));
  DEBUG((EFI_D_INFO, "CNTHCTL_EL2\t0x%lx\n", State->Cnthctl));
  DEBUG((EFI_D_INFO, "CNTVOFF_EL2\t0x%lx\n", State->Cntvoff));
  DEBUG((EFI_D_INFO, "VPIDR_EL2  \t0x%lx\n", State->Vpidr));
  DEBUG((EFI_D_INFO, "VTTBR_EL2  \t0x%lx\n", State->Vttbr));
  DEBUG((EFI_D_INFO, "VTCR_EL2   \t0x%lx\n", State->Vtcr));
  DEBUG((EFI_D_INFO, "SCTLR_EL2  \t0x%lx\n", State->Sctlr));
  DEBUG((EFI_D_INFO, "ACTLR_EL2  \t0x%lx\n", State->Actlr));
  DEBUG((EFI_D_INFO, "TTBR0_EL2  \t0x%lx\n", State->Ttbr0));
  DEBUG((EFI_D_INFO, "TCR_EL2    \t0x%lx\n", State->Tcr));
  DEBUG((EFI_D_INFO, "VBAR_EL2   \t0x%lx\n", State->Vbar));
  DEBUG((EFI_D_INFO, "MAIR_EL2   \t0x%lx\n", State->Mair));

  StackSize = PcdGet32(PcdCPUCorePrimaryStackSize);
  Stack = HypMemAlloc(EFI_SIZE_TO_PAGES(StackSize));
  if (Stack == 0) {
    DEBUG((EFI_D_ERROR, "No memory for stack\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  DEBUG((EFI_D_INFO, "%u of EL2 stack at %p\n", StackSize, Stack));

  /* EL1 is 64-bit, and we trap SMC calls. */
  WriteSysReg(hcr_el2, HCR_RW_64 | HCR_TSC | HCR_AMO | HCR_VM);
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
    asm volatile("hvc #0xdead");
  }

  Status = HypBuildS2PT(State);
  if (Status != EFI_SUCCESS) {
    asm volatile("hvc #0xdead");
  }

  return EFI_SUCCESS;
}


STATIC VOID
HypSwitchToEL1(IN  CAPTURED_EL2_STATE *State,
               IN  EFI_PHYSICAL_ADDRESS ExceptionStack)
{

  UINT64 Tcr;
  UINT64 Sctlr;
  UINT64 Spsel;

  DEBUG((EFI_D_INFO, "Switching to EL1\n"));

  ReadSysReg(Spsel, spsel);
  ASSERT (Spsel != 0);

  Sctlr =
    I(X(State->Sctlr, 0, 2), 0, 2)     | // C, A, M
    I(X(State->Sctlr, 12, 12), 12, 12) | // I
    SCTLR_EL1_RES1;
  WriteSysReg(sctlr_el1, Sctlr);

  WriteSysReg(ttbr0_el1, State->Ttbr0);
  WriteSysReg(ttbr1_el1, 0);

  Tcr =
    I(X(State->Tcr, 0, 5), 0, 5)     | // T0SZ
    I(X(State->Tcr, 8, 9), 8, 9)     | // IRGN0
    I(X(State->Tcr, 10, 11), 10, 11) | // ORNG0
    I(X(State->Tcr, 12, 13), 12, 13) | // SH0
    I(X(State->Tcr, 14, 15), 14, 15) | // TG0
    I(1, 23, 23)                     | // EPD1
    I(X(State->Tcr, 16, 18), 32, 34) | // PS -> IPS
    I(X(State->Tcr, 20, 20), 37, 37);  // TBI -> TBI0
  WriteSysReg(tcr_el1, Tcr);
  WriteSysReg(vbar_el1, State->Vbar);
  WriteSysReg(mair_el1, State->Mair);
  WriteSysReg(spsr_el2, SPSR_D | SPSR_A | SPSR_I |
              SPSR_F | SPSR_EL1 | Spsel);
  ISB();

  asm volatile("tlbi alle1\n\t"
               "dsb sy\n\t");
  ISB();

  SwitchStackAndEL(ExceptionStack);

  DEBUG((EFI_D_INFO, "Switched to EL1\n"));
}


STATIC VOID
HypExceptionFatal (
  IN     EFI_EXCEPTION_TYPE ExceptionType,
  IN     UINT64 FaultingGPA,
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
  DEBUG((EFI_D_ERROR, "ESR 0x%lx (EC 0x%x ISS 0x%x)\n",
         SystemContext->ESR,
         ESR_2_EC(SystemContext->ESR),
         ESR_2_ISS(SystemContext->ESR)));
  DEBUG((EFI_D_ERROR, "FAR = 0x%lx\n",
         SystemContext->FAR));
  if (SPSR_2_EL(SystemContext->SPSR) < 2) {
    DEBUG((EFI_D_ERROR, "Faulting GPA = 0x%lx\n",
           FaultingGPA));
  }
  while(1);
}


VOID
HypExceptionHandler (
  IN     EFI_EXCEPTION_TYPE ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT_AARCH64 *SystemContext
)
{
  GPA FaultingGPA;

  ReadSysReg(FaultingGPA, hpfar_el2);
  FaultingGPA = HPFAR_2_GPA(FaultingGPA);

  if (SPSR_2_BITNESS(SystemContext->SPSR) == 32 ||
      SPSR_2_EL(SystemContext->SPSR) == 2) {
    HypExceptionFatal(ExceptionType,
                      FaultingGPA,
                      SystemContext);
  }

  switch (ESR_2_EC(SystemContext->ESR)) {
  case ESR_EC_DABT_LO:
    if (FaultingGPA >= MMIO_EMU_START &&
        HypMmio(SystemContext) == EFI_SUCCESS) {
      SystemContext->ELR += 4;
      break;
    }

    if (HypMemIsHypAddr(GPA_2_MPA(FaultingGPA))) {
      /*
       * Ignore it.
       *
       * This is not just a malicious EL1, this could
       * be RuntimeDxe trying to "move" us, which is
       * not something we care about, because we're
       * not a runtime servce - we just want our
       * memory ranges to be reported that way.
       */
      SystemContext->ELR += 4;
      break;
    }

    /*
     * Fall-through.
     */
  case ESR_EC_IABT_LO: {
    UINT64 Hcr;

    ReadSysReg(Hcr, hcr_el2);
    /*
     * Do an EA.
     */
    DEBUG((EFI_D_ERROR, "Unhandled access to 0x%lx, injecting EA\n",
           FaultingGPA));
    WriteSysReg(far_el1, SystemContext->FAR);
    WriteSysReg(hcr_el2, Hcr | HCR_VSE);
  } break;
  case ESR_EC_HVC64:
    DEBUG((EFI_D_ERROR, "Hello from EL%u PC 0x%016lx SP 0x%016lx\n",
           SPSR_2_EL(SystemContext->SPSR),
           SystemContext->ELR, SystemContext->SP));
    break;
  case ESR_EC_SMC64: {
    HypWSTryPatch(SystemContext);
    HypSMCProcess(SystemContext);

    SystemContext->ELR += 4;
  } break;

  default:
    HypExceptionFatal(ExceptionType, FaultingGPA,
                      SystemContext);
  }
}


EFI_STATUS
EFIAPI
HypInitialize(
  IN  EFI_HANDLE ImageHandle,
  IN  EFI_SYSTEM_TABLE *SystemTable
  )
{
  UINT64 DAIF;
  CAPTURED_EL2_STATE State;
  EFI_PHYSICAL_ADDRESS ExceptionStack;
  EFI_STATUS Status;
  UINT32 DoEL1;

  if (ArmReadCurrentEL() != AARCH64_EL2) {
    DEBUG((EFI_D_INFO, "Not in EL2, nothing to do\n"));
    return EFI_UNSUPPORTED;
  }

  DoEL1 = PcdGet32(PcdBootInEL1);
  if (!DoEL1) {
    DEBUG((EFI_D_INFO, "Boot doesn't require EL1\n"));
    return EFI_UNSUPPORTED;
  }

  Status = HypMemInit(ImageHandle);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Could't initialize hypervisor memory\n"));
    return Status;
  }

  HypWSInit();

  ReadSysReg(DAIF, daif);
  ArmDisableAllExceptions();

  CaptureEL2State(&State);
  Status = HypModeInit(&State, &ExceptionStack);
  if (Status == EFI_SUCCESS) {
    HypSwitchToEL1(&State, ExceptionStack);
    /*
     * Once we come back here, HypDxe
     * code/data are RO and any attempts
     * to modify will be ignored.
     */
  }

  WriteSysReg(daif, DAIF);
  return EFI_SUCCESS;
}

