/** @file
*
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

#ifndef _MMC_HOST_DXE_H_
#define _MMC_HOST_DXE_H_

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DmaLib.h>

#include <Protocol/EmbeddedExternalDevice.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/MmcHost.h>
#include <Protocol/RaspberryPiFirmware.h>

#include <IndustryStandard/Bcm2836.h>
#include <IndustryStandard/Bcm2836MMCHS.h>
#include <IndustryStandard/RpiFirmware.h>

// #include <LedLib.h>

#define MAX_RETRY_COUNT (1000 * 20)

#define STALL_AFTER_SEND_CMD_US (200) // in microseconds
#define STALL_AFTER_REC_RESP_US (50)
#define STALL_AFTER_WRITE_US (200)
#define STALL_AFTER_READ_US (20)
#define STALL_AFTER_RETRY_US (20)

#define HC_MMC_CSD_GET_DEVICESIZE(Response)    ((Response[1] >> 16) | ((Response[2] & 0x3F) << 16));

#define MAX_DIVISOR_VALUE 1023

#endif
