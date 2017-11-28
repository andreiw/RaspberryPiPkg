#
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
#  Copyright (c) 2014-2016, Linaro Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

#include <AsmMacroIoLib.h>
#include <Library/ArmLib.h>
#include <IndustryStandard/Bcm2836.h>
#include <IndustryStandard/RpiFirmware.h>

#define MAX_TRIES     0x100000

    .macro  drain
    mov     r5, #MAX_TRIES
0:  ldr     r6, [r4, #BCM2836_MBOX_STATUS_OFFSET]
    tst     r6, #(1 << BCM2836_MBOX_STATUS_EMPTY)
    bne     1f
    dmb
    ldr     ip, [r4, #BCM2836_MBOX_READ_OFFSET]
    subs    r5, r5, #1
    bne     0b
1:
    .endm

    .macro  poll, status
    mov     r5, #MAX_TRIES
0:  ldr     r6, [r4, #BCM2836_MBOX_STATUS_OFFSET]
    tst     r6, #(1 << \status)
    beq     1f
    dmb
    subs    r5, r5, #1
    bne     0b
1:
    .endm

ASM_FUNC(ArmPlatformPeiBootAction)
    adr     r0, .Lmeminfo_buffer
    mov     r1, #FixedPcdGet64 (PcdDmaDeviceOffset)
    orr     r0, r0, #RPI_FW_MBOX_CHANNEL
    add     r0, r0, r1

    MOV32   (r4, BCM2836_MBOX_BASE_ADDRESS)

    drain
    poll    BCM2836_MBOX_STATUS_FULL
    str     r0, [r4, #BCM2836_MBOX_WRITE_OFFSET]
    dmb
    poll    BCM2836_MBOX_STATUS_EMPTY
    dmb
    ldr     r0, [r4, #BCM2836_MBOX_READ_OFFSET]
    dmb

    ldr     r0, .Lmemsize
    sub     r0, r0, #1
    mov     r1, #0
    ADRL    (r2, mSystemMemoryEnd)
    strd    r0, r1, [r2]
    bx      lr

    .align  4
.Lmeminfo_buffer:
    .long   .Lbuffer_size
    .long   0x0
    .long   RPI_FW_GET_ARM_MEMSIZE
    .long   8                           // buf size
    .long   0                           // input len
    .long   0                           // mem base
.Lmemsize:
    .long   0                           // mem size
    .long   0                           // end tag
    .set    .Lbuffer_size, . - .Lmeminfo_buffer

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ASM_FUNC(ArmPlatformGetPrimaryCoreMpId)
    MOV32  (r0, FixedPcdGet32 (PcdArmPrimaryCore))
    bx     lr

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_FUNC(ArmPlatformIsPrimaryCore)
    mov   r0, #1
    bx    lr

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
// With this function: CorePos = (ClusterId * 4) + CoreId
ASM_FUNC(ArmPlatformGetCorePosition)
    and   r1, r0, #ARM_CORE_MASK
    and   r0, r0, #ARM_CLUSTER_MASK
    add   r0, r1, r0, LSR #6
    bx    lr
