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

#ifndef ARM_DEFS_H
#define ARM_DEFS_H

#define ReadSysReg(var, reg) asm volatile("mrs %0, " #reg : "=r" (var))
#define WriteSysReg(reg, val) asm volatile("msr " #reg ", %0"  : : "r" (val))
#define ATS12E1R(what) asm volatile("at s12e1r, %0" : :"r" (what))

#define ISB() asm volatile("isb");
#define DSB_ISH() asm volatile("dsb ish");

#define SPSR_2_EL(spsr) (X((spsr), 2, 3))
#define SPSR_2_BITNESS(spsr) (X((spsr), 4, 4) ? 32 : 64)
#define SPSR_2_SPSEL(spsr) (X((spsr), 0, 0) ? 1 : 0)

#define HPFAR_2_GPA(hpfar) I(X((hpfar), 4, 39), 12, 47)
#define ESR_EC_HVC64   0x16
#define ESR_EC_SMC64   0x17
#define ESR_EC_IABT_LO 0x20
#define ESR_EC_DABT_LO 0x24
#define ESR_2_IL(x)  (X((x), 25, 25))
#define ESR_2_EC(x)  (X((x), 26, 31))
#define ESR_2_ISS(x) (X((x), 0, 24))

#define HCR_VM    BIT0
#define HCR_AMO   BIT5
#define HCR_VSE   BIT8
#define HCR_TSC   BIT19
#define HCR_RW_64 BIT31
#define SPSR_D    BIT9
#define SPSR_EL1  0x4
#define SPSR_ELx  0x1

#define SCTLR_EL1_RES1 (BIT29 | BIT28 | BIT23 | \
                        BIT22 | BIT20 | BIT11)
/*
 * I share UEFI's MAIR.
 */
#define PTE_ATTR_MEM    TT_ATTR_INDX_MEMORY_WRITE_BACK
#define PTE_ATTR_DEV    TT_ATTR_INDX_DEVICE_MEMORY
/*
 * Device is GRE (to allow EL1 to do whatever).
 * Memory is Inner-WB Outer-WB.
 */
#define PTE_S2_ATTR_MEM (I(3, 4, 5) | I(3, 2, 3))
#define PTE_S2_ATTR_DEV (I(0, 4, 5) | I(3, 2, 3))
#define PTE_RW          I(0x1, 6, 7)
#define PTE_S2_RW       I(0x3, 6, 7)
#define PTE_S2_RO       I(0x1, 6, 7)
#define PTE_SH_INNER    I(0x3, 8, 9)
#define PTE_AF          I(0x1, 10, 10)

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
#define VA_2_PL3_IX(x)    X((x), 20, 12)

#define PAR_IS_BAD(par)   (((par) & 1) == 1)
#define PAR_2_ADDR(par)   M((par), 47, 12)

#define TLBI_S12()        do {       \
    ISB();                           \
    asm volatile("tlbi vmalls12e1"); \
    DSB_ISH();                       \
    ISB();                           \
  } while (0)

#define PSCI_CPU_ON_64               0xC4000003
#define PSCI_CPU_OFF                 0x84000002
#define PSCI_RETURN_STATUS_SUCCESS   0
#define PSCI_RETURN_STATUS_DENIED    -3
#define PSCI_RETURN_INTERNAL_FAILURE -6

#endif /* ARM_DEFS_H */
