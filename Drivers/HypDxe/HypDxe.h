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

#ifndef HYP_DXE_H
#define HYP_DXE_H

#include <Uefi.h>
#include <Chipset/AArch64.h>
#include <Protocol/DebugSupport.h>

EFI_STATUS InstallHiiPages(VOID);

typedef struct HYP_EXC_CONTEXT {
  EFI_SYSTEM_CONTEXT_AARCH64 Sys;
  UINT64 PAR;
  UINT64 Padding;
} HYP_EXC_CONTEXT;

extern void *ExceptionHandlersStart;

#endif /* HYP_DXE_H */
