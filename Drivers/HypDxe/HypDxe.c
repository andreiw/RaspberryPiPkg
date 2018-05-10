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
#include <Chipset/AArch64.h>

#define HCR_RW_64 BIT31
#define SPSR_D    BIT9
#define SPSR_EL1  0x4
#define SPSR_ELx  0x1
#define ReadSysReg(var, reg) asm volatile("mrs %0, " #reg : "=r" (var))
#define WriteSysReg(reg, val) asm volatile("msr " #reg ", %0"  : : "r" (val))
#define ISB() asm volatile("isb");

#define _IX_BITS(sm, bg) (bg - sm + 1)
#define _IX_MASK(sm, bg) ((1 << _IX_BITS(sm, bg)) - 1)
#define _X(val, sm, bg) (val >> sm) & _IX_MASK(sm, bg)
#define X(val, ix1, ix2) ((ix1 < ix2) ? _X(val, ix1, ix2) :     \
                          _X(val, ix2, ix1))

#define _I(val, sm, bg)  ((val & _IX_MASK(sm, bg)) << sm)
#define I(val, ix1, ix2) ((ix1 < ix2) ? _I(val, ix1, ix2) :     \
                          _I(val, ix2, ix1))


STATIC
VOID
HypSwitchToEL1(void)
{
  UINT64 daif;
  UINT64 hcr;
  UINT64 cptr;
  UINT64 cnthctl;
  UINT64 cntvoff;
  UINT64 vpidr;
  UINT64 vmpidr;
  UINT64 vttbr;
  UINT64 vtcr;
  UINT64 sctlr;
  UINT64 actlr;
  UINT64 tcr;
  UINT64 ttbr0;
  UINT64 vbar;
  UINT64 mair;
  UINT64 spsel;

  UINT64 tcr_el1;
  UINT64 sctlr_el1;
  UINT64 sp = 0;

  ReadSysReg(daif, daif);
  ReadSysReg(hcr, hcr_el2);
  ReadSysReg(cptr, cptr_el2);
  ReadSysReg(cnthctl, cnthctl_el2);
  ReadSysReg(cntvoff, cntvoff_el2);
  ReadSysReg(vpidr, vpidr_el2);
  ReadSysReg(vmpidr, vmpidr_el2);
  ReadSysReg(vttbr, vttbr_el2);
  ReadSysReg(vtcr, vtcr_el2);
  ReadSysReg(sctlr, sctlr_el2);
  ReadSysReg(actlr, actlr_el2);
  ReadSysReg(ttbr0, ttbr0_el2);
  ReadSysReg(tcr, tcr_el2);
  ReadSysReg(vbar, vbar_el2);
  ReadSysReg(mair, mair_el2);
  ReadSysReg(spsel, spsel);

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

  DEBUG((EFI_D_INFO, "HCR_EL2    \t0x%lx\n", hcr));
  DEBUG((EFI_D_INFO, "CPTR_EL2   \t0x%lx\n", cptr));
  DEBUG((EFI_D_INFO, "CNTHCTL_EL2\t0x%lx\n", cnthctl));
  DEBUG((EFI_D_INFO, "CNTVOFF_EL2\t0x%lx\n", cntvoff));
  DEBUG((EFI_D_INFO, "VPIDR_EL2  \t0x%lx\n", vpidr));
  DEBUG((EFI_D_INFO, "VMPIDR_EL2 \t0x%lx\n", vmpidr));
  DEBUG((EFI_D_INFO, "VTTBR_EL2  \t0x%lx\n", vttbr));
  DEBUG((EFI_D_INFO, "VTCR_EL2   \t0x%lx\n", vtcr));
  DEBUG((EFI_D_INFO, "SCTLR_EL2  \t0x%lx\n", sctlr));
  DEBUG((EFI_D_INFO, "ACTLR_EL2  \t0x%lx\n", actlr));
  DEBUG((EFI_D_INFO, "TTBR0_EL2  \t0x%lx\n", ttbr0));
  DEBUG((EFI_D_INFO, "TCR_EL2    \t0x%lx\n", tcr));
  DEBUG((EFI_D_INFO, "VBAR_EL2   \t0x%lx\n", vbar));
  DEBUG((EFI_D_INFO, "MAIR_EL2   \t0x%lx\n", mair));
  DEBUG((EFI_D_INFO, "SPSEL      \t0x%lx\n", spsel));

  ArmDisableAllExceptions();

  WriteSysReg(hcr_el2, HCR_RW_64);
  sctlr_el1 =
    I(X(sctlr, 0, 2), 0, 2)     | // C, A, M
    I(1, 12, 12)                | // RES1
    I(X(sctlr, 12, 12), 12, 12) | // I
    I(1, 20, 20)                | // RES1
    I(0x3, 22, 23)              | // RES1
    I(0x3, 28, 29);               // RES1
  WriteSysReg(sctlr_el1, sctlr_el1);

  WriteSysReg(ttbr0_el1, ttbr0);
  WriteSysReg(ttbr1_el1, 0);

  tcr_el1 =
    I(X(tcr, 0, 5), 0, 5)     | // T0SZ
    I(X(tcr, 8, 9), 8, 9)     | // IRGN0
    I(X(tcr, 10, 11), 10, 11) | // ORNG0
    I(X(tcr, 12, 13), 12, 13) | // SH0
    I(X(tcr, 14, 15), 14, 15) | // TG0
    I(1, 23, 23)              | // EPD1
    I(X(tcr, 16, 18), 32, 34) | // PS -> IPS
    I(X(tcr, 20, 20), 37, 37);  // TBI -> TBI0
  WriteSysReg(tcr_el1, tcr_el1);
  WriteSysReg(vbar_el1, vbar);
  WriteSysReg(mair_el1, mair);
  WriteSysReg(elr_el2, &&done);
  WriteSysReg(spsr_el2, SPSR_D | SPSR_A | SPSR_I |
              SPSR_F | SPSR_EL1 | SPSR_ELx);
  ISB();

  asm volatile("tlbi alle1\n\t"
               "dsb sy\n\t");
  ISB();

  asm volatile("mov %0, sp\n\t"
               "msr sp_el1, %0\n\t"
               "eret\n\t" : "+r"(sp));
 done:
  WriteSysReg(daif, daif);
}


EFI_STATUS
EFIAPI
HypInitialize(
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  UINT32 doEL1;

  if (ArmReadCurrentEL () != AARCH64_EL2) {
    return EFI_UNSUPPORTED;
  }

  doEL1 = PcdGet32 (PcdBootInEL1);
  if (doEL1) {
    DEBUG((EFI_D_INFO, "Switching to EL1\n"));
    HypSwitchToEL1();
    DEBUG((EFI_D_INFO, "In EL1\n"));
  }

  /*
   * Update regardless to create NV variable. The
   * config VFR won't work otherwise, failing to save.
   */
  PcdSet32 (PcdBootInEL1, doEL1);

  Status = InstallHiiPages();
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "Couldn't install HypDxe configuration pages: %r\n",
           Status));
  }

  return EFI_SUCCESS;
}

