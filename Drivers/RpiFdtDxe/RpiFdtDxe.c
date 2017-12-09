/** @file
  Copyright (c), 2017, Andrey Warkentin <andrey.warkentin@gmail.com>
  Copyright (c) 2016, Linaro, Ltd. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <libfdt.h>

#include <Protocol/RaspberryPiFirmware.h>

#include <Guid/Fdt.h>

STATIC VOID                             *mFdtImage;

STATIC RASPBERRY_PI_FIRMWARE_PROTOCOL   *mFwProtocol;

STATIC
VOID
UpdateMacAddress (
  VOID
  )
{
  INTN          Node;
  INTN          Retval;
  EFI_STATUS    Status;
  UINT8         MacAddress[6];

  //
  // Locate the node that the 'ethernet' alias refers to
  //
  Node = fdt_path_offset(mFdtImage, "ethernet");
  if (Node < 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate 'ethernet' alias\n",
      __FUNCTION__));
    return;
  }

  //
  // Get the MAC address from the firmware
  //
  Status = mFwProtocol->GetMacAddress (MacAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to retrieve MAC address\n", __FUNCTION__));
    return;
  }

  Retval = fdt_setprop (mFdtImage, Node, "mac-address", MacAddress,
    sizeof MacAddress);
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to create 'mac-address' property (%d)\n",
      __FUNCTION__, Retval));
      return;
  }

  DEBUG ((DEBUG_INFO,
    "%a: setting MAC address to %02x:%02x:%02x:%02x:%02x:%02x\n",
    __FUNCTION__, MacAddress[0], MacAddress[1], MacAddress[2], MacAddress[3],
    MacAddress[4], MacAddress[5]));
}

#define MAX_CMDLINE_SIZE    512

STATIC
VOID
UpdateBootArgs (
  VOID
  )
{
  INTN          Node;
  INTN          Retval;
  EFI_STATUS    Status;
  CHAR8         *CommandLine;

  //
  // Locate the /chosen node
  //
  Node = fdt_path_offset(mFdtImage, "/chosen");
  if (Node < 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate /chosen node\n",
      __FUNCTION__));
    return;
  }

  //
  // If /chosen/bootargs already exists, we want to add a space character
  // before adding the firmware supplied arguments. However, the RpiFirmware
  // protocol expects a 32-bit aligned buffer. So let's allocate 4 bytes of
  // slack, and skip the first 3 when passing this buffer into libfdt.
  //
  CommandLine = AllocatePool (MAX_CMDLINE_SIZE) + 4;
  if (!CommandLine) {
    DEBUG ((DEBUG_ERROR, "%a: failed to allocate memory\n", __FUNCTION__));
    return;
  }

  //
  // Get the command line from the firmware
  //
  Status = mFwProtocol->GetCommandLine (MAX_CMDLINE_SIZE, CommandLine + 4);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to retrieve command line\n",
      __FUNCTION__));
    return;
  }

  if (AsciiStrLen (CommandLine + 4) == 0) {
    DEBUG ((DEBUG_INFO, "%a: empty command line received\n", __FUNCTION__));
    return;
  }

  CommandLine[3] = ' ';

  Retval = fdt_appendprop_string (mFdtImage, Node, "bootargs", &CommandLine[3]);
  if (Retval != 0) {
    DEBUG ((DEBUG_ERROR, "%a: failed to set /chosen/bootargs property (%d)\n",
      __FUNCTION__, Retval));
  }

  DEBUG_CODE_BEGIN ();
    CONST VOID    *Prop;
    INT32         Length;

    Node = fdt_path_offset (mFdtImage, "/chosen");
    ASSERT (Node >= 0);

    Prop = fdt_getprop (mFdtImage, Node, "bootargs", &Length);
    ASSERT (Prop != NULL);

    DEBUG ((DEBUG_INFO, "Command line set from firmware (length %d)\n", Length));
    DEBUG ((DEBUG_INFO, "'%a'\n", &CommandLine[3]));
  DEBUG_CODE_END ();

  FreePool (CommandLine - 4);
}


/**
  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
EFIAPI
RpiFdtDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;
  VOID       *FdtImage;
  UINTN      FdtSize;
  INT32      Retval;

  Status = gBS->LocateProtocol (&gRaspberryPiFirmwareProtocolGuid, NULL,
                  (VOID **)&mFwProtocol);
  ASSERT_EFI_ERROR (Status);

  FdtImage =  (VOID *) (UINTN) PcdGet32(PcdFdtBaseAddress);
  Retval = fdt_check_header (FdtImage);
  if (Retval == 0) {
    /*
     * Have FDT passed via config.txt.
     */
    FdtSize = fdt_totalsize (FdtImage);
    DEBUG ((DEBUG_INFO, "DTB passed via config.txt of 0x%lx bytes\n", FdtSize));
    Status = EFI_SUCCESS;
  } else {
    DEBUG ((DEBUG_INFO, "No/bad FDT at %p (%a), trying internal DTB...\n",
            FdtImage, fdt_strerror (Retval)));
    Status = GetSectionFromAnyFv (&gRaspberryPiFdtFileGuid, EFI_SECTION_RAW, 0,
                                  &FdtImage, &FdtSize);
    if (Status == EFI_SUCCESS) {
      if (fdt_check_header (FdtImage) != 0) {
        Status = EFI_INCOMPATIBLE_VERSION;
      }
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate device tree: %r\n", Status));
    return Status;
  }

  /*
   * Probably overkill.
   */
  FdtSize += EFI_PAGE_SIZE * 2;
  Status = gBS->AllocatePages (AllocateAnyPages, EfiRuntimeServicesData,
                               EFI_SIZE_TO_PAGES(FdtSize),
                               (EFI_PHYSICAL_ADDRESS *) &mFdtImage);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate new device tree: %r\n", Status));
    return Status;
  }

  Retval = fdt_open_into (FdtImage, mFdtImage, FdtSize);
  ASSERT (Retval == 0);

  UpdateMacAddress ();
  UpdateBootArgs ();

  Status = gBS->InstallConfigurationTable (&gFdtTableGuid, mFdtImage);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
