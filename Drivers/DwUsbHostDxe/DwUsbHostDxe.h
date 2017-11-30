/** @file

  Copyright (c) 2015-2016, Linaro Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DWUSBHOSTDXE_H_
#define _DWUSBHOSTDXE_H_

#include <Uefi.h>

#include <Protocol/DwUsb.h>
#include <Protocol/Usb2HostController.h>

#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/DmaLib.h>

#define MAX_DEVICE                      16
#define MAX_ENDPOINT                    16

typedef struct _DWUSB_OTGHC_DEV DWUSB_OTGHC_DEV;

#define DWUSB_OTGHC_DEV_SIGNATURE	SIGNATURE_32 ('d', 'w', 'h', 'c')
#define DWHC_FROM_THIS(a)		CR(a, DWUSB_OTGHC_DEV, DwUsbOtgHc, DWUSB_OTGHC_DEV_SIGNATURE)

//
// The RequestType in EFI_USB_DEVICE_REQUEST is composed of
// three fields: One bit direction, 2 bit type, and 5 bit
// target.
//
#define USB_REQUEST_TYPE(Dir, Type, Target) \
          ((UINT8)((((Dir) == EfiUsbDataIn ? 0x01 : 0) << 7) | (Type) | (Target)))

typedef struct {
	ACPI_HID_DEVICE_PATH		AcpiDevicePath;
	PCI_DEVICE_PATH			PciDevicePath;
	EFI_DEVICE_PATH_PROTOCOL	EndDevicePath;
} EFI_USB_PCIIO_DEVICE_PATH;

struct _DWUSB_OTGHC_DEV {
        UINTN                           Signature;
        EFI_HANDLE                      DeviceHandle;

        EFI_USB2_HC_PROTOCOL            DwUsbOtgHc;

        EFI_USB_HC_STATE                DwHcState;

        EFI_USB_PCIIO_DEVICE_PATH       DevicePath;

        EFI_EVENT                       ExitBootServiceEvent;

        EFI_PHYSICAL_ADDRESS            DwUsbBase;
        UINT8                           *StatusBuffer;

        UINT8                           *AlignedBuffer;
        VOID *                          AlignedBufferMapping;
        UINTN                           AlignedBufferBusAddress;

        UINT16                          PortStatus;
        UINT16                          PortChangeStatus;

        UINT32                          RhDevNum;
};

#endif //_DWUSBHOSTDXE_H_
