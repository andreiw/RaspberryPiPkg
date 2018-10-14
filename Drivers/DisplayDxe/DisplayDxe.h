/** @file
 *
 *  Copyright (c) 2017 - 2018, Andrei Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (c), Microsoft Corporation. All rights reserved.
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

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/DevicePath.h>
#include <Protocol/RaspberryPiFirmware.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/Cpu.h>
#include <Utils.h>

extern EFI_GRAPHICS_OUTPUT_PROTOCOL gDisplayProto;
extern EFI_COMPONENT_NAME_PROTOCOL  gComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gComponentName2;

VOID
RegisterScreenshotHandlers(
  VOID
  );

#endif /* _DISPLAY_H_ */
