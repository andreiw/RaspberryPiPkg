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
  ReadSysReg(State->Mdcr, mdcr_el2);
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
    HLOG((HLOG_ERROR, "Couldn't alloc S2 L1 table\n"));
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
        HLOG((HLOG_ERROR, "Couldn't alloc S2 L2 table for %u\n",
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
          HLOG((HLOG_ERROR, "Couldn't alloc S2 L3 table for %u\n",
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
  HLOG((HLOG_INFO, "Setting S2 page table root to 0x%lx\n",
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
    HLOG((HLOG_ERROR, "Couldn't alloc L1 table\n"));
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
        HLOG((HLOG_ERROR, "Couldn't alloc L2 table for %u\n",
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

  HLOG((HLOG_INFO, "Setting page table root to 0x%lx\n",
        (UINT64) PL1));

  DSB_ISH();
  WriteSysReg(ttbr0_el2, PL1);
  ISB();

  asm volatile("tlbi alle2");
  DSB_ISH();
  ISB();

  HLOG((HLOG_INFO, "It lives!\n"));

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

  HLOG((HLOG_INFO, "HCR_EL2    \t0x%lx\n", State->Hcr));
  HLOG((HLOG_INFO, "CPTR_EL2   \t0x%lx\n", State->Cptr));
  HLOG((HLOG_INFO, "CNTHCTL_EL2\t0x%lx\n", State->Cnthctl));
  HLOG((HLOG_INFO, "CNTVOFF_EL2\t0x%lx\n", State->Cntvoff));
  HLOG((HLOG_INFO, "VPIDR_EL2  \t0x%lx\n", State->Vpidr));
  HLOG((HLOG_INFO, "VTTBR_EL2  \t0x%lx\n", State->Vttbr));
  HLOG((HLOG_INFO, "VTCR_EL2   \t0x%lx\n", State->Vtcr));
  HLOG((HLOG_INFO, "SCTLR_EL2  \t0x%lx\n", State->Sctlr));
  HLOG((HLOG_INFO, "ACTLR_EL2  \t0x%lx\n", State->Actlr));
  HLOG((HLOG_INFO, "TTBR0_EL2  \t0x%lx\n", State->Ttbr0));
  HLOG((HLOG_INFO, "TCR_EL2    \t0x%lx\n", State->Tcr));
  HLOG((HLOG_INFO, "VBAR_EL2   \t0x%lx\n", State->Vbar));
  HLOG((HLOG_INFO, "MAIR_EL2   \t0x%lx\n", State->Mair));

  StackSize = PcdGet32(PcdCPUCorePrimaryStackSize);
  Stack = HypMemAlloc(EFI_SIZE_TO_PAGES(StackSize));
  if (Stack == 0) {
    HLOG((HLOG_ERROR, "No memory for stack\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  HLOG((HLOG_INFO, "%u of EL2 stack at %p\n", StackSize, Stack));

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

  HLOG((HLOG_INFO, "Switching to EL1\n"));

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

  HLOG((HLOG_INFO, "Switched to EL1\n"));
}


STATIC VOID
HypExceptionFatal (
  IN     EFI_EXCEPTION_TYPE ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT_AARCH64 *SystemContext
)
{
  UINTN EL = SPSR_2_EL(SystemContext->SPSR);

  if (SPSR_2_BITNESS(SystemContext->SPSR) == 32) {
    HLOG((HLOG_ERROR, "Unexpected exception from AArch32!\n"));
    goto done;
  }

  HLOG((HLOG_ERROR, "Unexpected exception from EL%u!\n", EL));

done:
  HLOG((HLOG_ERROR, "ESR 0x%lx (EC 0x%x ISS 0x%x)\n",
         SystemContext->ESR,
         ESR_2_EC(SystemContext->ESR),
         ESR_2_ISS(SystemContext->ESR)));
  HLOG((HLOG_ERROR, "FAR = 0x%lx\n",
         SystemContext->FAR));

  if (EL < 2) {
    UINT64 Hcr;
    ReadSysReg(Hcr, hcr_el2);

    /*
     * Do an EA.
     */
    WriteSysReg(far_el1, SystemContext->FAR);
    WriteSysReg(hcr_el2, Hcr | HCR_VSE);
    HLOG((HLOG_ERROR, "Injecting EA!\n"));
    return;
  }

  while(1);
}


VOID
HypExceptionHandler (
  IN     EFI_EXCEPTION_TYPE ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT_AARCH64 *SystemContext
)
{
  BOOLEAN Handled = FALSE;
  UINTN EC = ESR_2_EC(SystemContext->ESR);

  if (SPSR_2_BITNESS(SystemContext->SPSR) == 32 ||
      SPSR_2_EL(SystemContext->SPSR) == 2) {
    HypExceptionFatal(ExceptionType, SystemContext);
  }

  switch (EC) {
  case ESR_EC_IABT_LO:
  case ESR_EC_DABT_LO: {
    GPA FaultingGPA;
    ReadSysReg(FaultingGPA, hpfar_el2);
    FaultingGPA = HPFAR_2_GPA(FaultingGPA, SystemContext->FAR);

    if (EC == ESR_EC_DABT_LO) {
      if (FaultingGPA >= MMIO_EMU_START &&
          HypMmio(SystemContext) == EFI_SUCCESS) {
        SystemContext->ELR += 4;
        Handled = TRUE;
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
        Handled = TRUE;
        break;
      }
    }

    HLOG((HLOG_ERROR, "Faulting GPA = 0x%lx\n", FaultingGPA));
  } break;
  case ESR_EC_HVC64:
    HypHVCProcess(SystemContext);
    Handled = TRUE;
    break;
  case ESR_EC_SMC64: {
    HypWSTryPatch(SystemContext);
    HypSMCProcess(SystemContext);
    SystemContext->ELR += 4;
    Handled = TRUE;
  } break;
  case ESR_EC_MSR: {
    Handled = HypSYSProcess(SystemContext);
    if (Handled) {
      SystemContext->ELR += 4;
    }
  } break;
  case ESR_EC_BRK: {
    if (SPSR_2_EL(SystemContext->SPSR) < 2) {
      HypWSTryBRK(SystemContext);
      Handled = TRUE;
    } else {
      HLOG((HLOG_ERROR, "Unexpected BRK\n"));
    }
  } break;
  default:
    HLOG((HLOG_ERROR, "Unhandled fault reason 0x%lx\n",
          ESR_2_EC(SystemContext->ESR)));
  }

  if (!Handled) {
    HypExceptionFatal(ExceptionType, SystemContext);
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
  UINT32 HypEnable;

  Status = HypLogInit();
  if (EFI_ERROR(Status)) {
    return Status;
  }

  if (ArmReadCurrentEL() != AARCH64_EL2) {
    HLOG((HLOG_INFO, "Not in EL2, nothing to do\n"));
    return EFI_UNSUPPORTED;
  }

  HypEnable = PcdGet32(PcdHypEnable);
  if (!HypEnable) {
    HLOG((HLOG_INFO, "Boot doesn't require hypervisor\n"));
    return EFI_UNSUPPORTED;
  }

  Status = HypMemInit(ImageHandle);
  if (EFI_ERROR(Status)) {
    HLOG((HLOG_ERROR, "Couldn't initialize hypervisor memory\n"));
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

