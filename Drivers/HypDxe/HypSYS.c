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

#define PASS_THRU(reg) do {                 \
    UINT64 *R = &SystemContext->X0;         \
    if (Write) {                            \
      WriteSysReg(reg, R[ISS_SYS_Rt(ISS)]); \
    } else {                                \
      ReadSysReg(R[ISS_SYS_Rt(ISS)], reg);  \
    }                                       \
    return TRUE;                            \
  } while(0)

#define PASS_THRU_WO(reg) do {              \
    UINT64 *R = &SystemContext->X0;         \
    if (Write) {                            \
      WriteSysReg(reg, R[ISS_SYS_Rt(ISS)]); \
      return TRUE;                          \
    }                                       \
    return FALSE;                           \
  } while(0)

#define PASS_THRU_RO(reg) do {              \
    UINT64 *R = &SystemContext->X0;         \
    if (!Write) {                           \
      ReadSysReg(R[ISS_SYS_Rt(ISS)], reg);  \
      return TRUE;                          \
    }                                       \
    return FALSE;                           \
  } while(0)

BOOLEAN
HypSYSProcess(
  IN  EFI_SYSTEM_CONTEXT_AARCH64 *SystemContext
  )
{
  UINTN ISS = ESR_2_ISS(SystemContext->ESR);
  BOOLEAN Write = ISS_SYS_WRITE(ISS);

  if (!ISS_SYS_MSR(ISS)) {
    /*
     * Not MSR/MRS, but some other kind of system instruction
     * that we didn't expect.
     */
    return FALSE;
  }

  switch (ISS_SYS_2_MSRDEF(ISS)) {
  case MSRDEF_OSDTRRX_EL1: PASS_THRU(osdtrrx_el1);
  case MSRDEF_MDCCINT_EL1: PASS_THRU(mdccint_el1);
  case MSRDEF_MDSCR_EL1: PASS_THRU(mdscr_el1);
  case MSRDEF_OSDTRTX_EL1: PASS_THRU(osdtrtx_el1);
  case MSRDEF_OSECCR_EL1: PASS_THRU(oseccr_el1);
  case MSRDEF_DBGBVR_EL1(0): PASS_THRU(dbgbvr0_el1);
  case MSRDEF_DBGBVR_EL1(1): PASS_THRU(dbgbvr1_el1);
  case MSRDEF_DBGBVR_EL1(2): PASS_THRU(dbgbvr2_el1);
  case MSRDEF_DBGBVR_EL1(3): PASS_THRU(dbgbvr3_el1);
  case MSRDEF_DBGBVR_EL1(4): PASS_THRU(dbgbvr4_el1);
  case MSRDEF_DBGBVR_EL1(5): PASS_THRU(dbgbvr5_el1);
  case MSRDEF_DBGBCR_EL1(0): PASS_THRU(dbgbcr0_el1);
  case MSRDEF_DBGBCR_EL1(1): PASS_THRU(dbgbcr1_el1);
  case MSRDEF_DBGBCR_EL1(2): PASS_THRU(dbgbcr2_el1);
  case MSRDEF_DBGBCR_EL1(3): PASS_THRU(dbgbcr3_el1);
  case MSRDEF_DBGBCR_EL1(4): PASS_THRU(dbgbcr4_el1);
  case MSRDEF_DBGBCR_EL1(5): PASS_THRU(dbgbcr5_el1);
  case MSRDEF_DBGWVR_EL1(0): PASS_THRU(dbgwvr0_el1);
  case MSRDEF_DBGWVR_EL1(1): PASS_THRU(dbgwvr1_el1);
  case MSRDEF_DBGWVR_EL1(2): PASS_THRU(dbgwvr2_el1);
  case MSRDEF_DBGWVR_EL1(3): PASS_THRU(dbgwvr3_el1);
  case MSRDEF_DBGWVR_EL1(4): PASS_THRU(dbgwvr4_el1);
  case MSRDEF_DBGWVR_EL1(5): PASS_THRU(dbgwvr5_el1);
  case MSRDEF_DBGWCR_EL1(0): PASS_THRU(dbgwcr0_el1);
  case MSRDEF_DBGWCR_EL1(1): PASS_THRU(dbgwcr1_el1);
  case MSRDEF_DBGWCR_EL1(2): PASS_THRU(dbgwcr2_el1);
  case MSRDEF_DBGWCR_EL1(3): PASS_THRU(dbgwcr3_el1);
  case MSRDEF_DBGWCR_EL1(4): PASS_THRU(dbgwcr4_el1);
  case MSRDEF_DBGWCR_EL1(5): PASS_THRU(dbgwcr5_el1);
  case MSRDEF_MDRAR_EL1: PASS_THRU_RO(mdrar_el1);
  case MSRDEF_OSLSR_EL1: PASS_THRU_RO(oslsr_el1);
  case MSRDEF_OSLAR_EL1: PASS_THRU_WO(oslar_el1);
  case MSRDEF_OSDLR_EL1: PASS_THRU(osdlr_el1);
  case MSRDEF_DBGPRCR_EL1: PASS_THRU(dbgprcr_el1);
  case MSRDEF_DBGCLAIMSET_EL1: PASS_THRU(dbgclaimset_el1);
  case MSRDEF_DBGCLAIMCLR_EL1: PASS_THRU(dbgclaimclr_el1);
  case MSRDEF_DBGAUTHSTAT_EL1: PASS_THRU_RO(dbgauthstatus_el1);
  case MSRDEF_MDCCSR_EL0: PASS_THRU_RO(mdccsr_el0);
  case MSRDEF_DBGDTR_EL0: PASS_THRU(dbgdtr_el0);
  case MSRDEF_DBGDTRF_EL0: PASS_THRU(dbgdtrrx_el0);
  }

  HLOG((HLOG_ERROR, "%a S%u_%u_%u_%u_%u\n",
        Write ? "Write to" : "Read from",
        ISS_SYS_O0(ISS), ISS_SYS_Op1(ISS),
        ISS_SYS_CRn(ISS), ISS_SYS_CRm(ISS),
        ISS_SYS_Op2(ISS)));

  return FALSE;
}
