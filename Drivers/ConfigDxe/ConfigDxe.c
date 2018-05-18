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
#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include "ConfigDxeFormSetGuid.h"

extern UINT8 ConfigDxeHiiBin[];
extern UINT8 ConfigDxeStrings[];

typedef struct {
  VENDOR_DEVICE_PATH VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL End;
} HII_VENDOR_DEVICE_PATH;

STATIC HII_VENDOR_DEVICE_PATH mVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    CONFIGDXE_FORM_SET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};


STATIC EFI_STATUS
InstallHiiPages (
  VOID
  )
{
  EFI_STATUS     Status;
  EFI_HII_HANDLE HiiHandle;
  EFI_HANDLE     DriverHandle;

  DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (&DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mVendorDevicePath,
                  NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiHandle = HiiAddPackages (&gConfigDxeFormSetGuid,
                              DriverHandle,
                              ConfigDxeStrings,
                              ConfigDxeHiiBin,
                              NULL);

  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mVendorDevicePath,
                  NULL);
    return EFI_OUT_OF_RESOURCES;
  }
  return EFI_SUCCESS;
}


STATIC EFI_STATUS
SetupVariables (
  VOID
  )
{
  UINTN Size;
  UINT32 BootInEL1;
  EFI_STATUS Status;

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"BootInEL1",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &BootInEL1);
  if (EFI_ERROR (Status)) {
    /*
     * Create the var. If we don't, forms won't
     * be able to update.
     */
    DEBUG((EFI_D_INFO, "-------- Created BootInEL1 variable\n"));
    PcdSet32 (PcdBootInEL1, 0);
  }

  return EFI_SUCCESS;
}
  

EFI_STATUS
EFIAPI
ConfigInitialize(
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;

  Status = SetupVariables();
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "Couldn't not setup NV vars: %r\n",
           Status));
  }

  Status = InstallHiiPages();
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "Couldn't install ConfigDxe configuration pages: %r\n",
           Status));
  }

  return EFI_SUCCESS;
}

