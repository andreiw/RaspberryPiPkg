#
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
#  Copyright (c) 2016, Linaro Limited. All rights reserved.
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

#include <AsmMacroIoLibV8.h>
#include <Library/ArmLib.h>
#include <IndustryStandard/Bcm2836.h>
#include <IndustryStandard/RpiFirmware.h>

#define MAX_TRIES     0x100000

    .macro  drain
    mov     x5, #MAX_TRIES
0:  ldr     w6, [x4, #BCM2836_MBOX_STATUS_OFFSET]
    tbnz    w6, #BCM2836_MBOX_STATUS_EMPTY, 1f
    dmb     ld
    ldr     wzr, [x4, #BCM2836_MBOX_READ_OFFSET]
    subs    x5, x5, #1
    b.ne    0b
1:
    .endm

    .macro  poll, status
    mov     x5, #MAX_TRIES
0:  ldr     w6, [x4, #BCM2836_MBOX_STATUS_OFFSET]
    tbz     w6, #\status, 1f
    dmb     ld
    subs    x5, x5, #1
    b.ne    0b
1:
    .endm

ASM_FUNC(ArmPlatformPeiBootAction)
    adr     x0, .Lmeminfo_buffer
    mov     x1, #FixedPcdGet64 (PcdDmaDeviceOffset)
    orr     x0, x0, #RPI_FW_MBOX_CHANNEL
    add     x0, x0, x1

    MOV32   (x4, BCM2836_MBOX_BASE_ADDRESS)

    drain
    poll    BCM2836_MBOX_STATUS_FULL
    str     w0, [x4, #BCM2836_MBOX_WRITE_OFFSET]
    dmb     sy
    poll    BCM2836_MBOX_STATUS_EMPTY
    dmb     sy
    ldr     wzr, [x4, #BCM2836_MBOX_READ_OFFSET]
    dmb     ld

    ldr     w0, .Lmemsize
    sub     x0, x0, #1
    adr     x1, mSystemMemoryEnd
    str     x0, [x1]
    ret

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
    MOV32  (w0, FixedPcdGet32 (PcdArmPrimaryCore))
    ret

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_FUNC(ArmPlatformIsPrimaryCore)
    mov   x0, #1
    ret

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
// With this function: CorePos = (ClusterId * 4) + CoreId
ASM_FUNC(ArmPlatformGetCorePosition)
    and   x1, x0, #ARM_CORE_MASK
    and   x0, x0, #ARM_CLUSTER_MASK
    add   x0, x1, x0, LSR #6
    ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
