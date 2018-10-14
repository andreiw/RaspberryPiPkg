/** @file
 *
 *  Copyright (c) 2018, Andrei Warkentin <andrey.warkentin@gmail.com>
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
#include <Library/GpioLib.h>
#include <Protocol/RaspberryPiFirmware.h>
#include <IndustryStandard/RpiFirmware.h>
#include "ConfigDxeFormSetGuid.h"

extern UINT8 ConfigDxeHiiBin[];
extern UINT8 ConfigDxeStrings[];

STATIC RASPBERRY_PI_FIRMWARE_PROTOCOL *mFwProtocol;

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
  UINT32 Var32;
  EFI_STATUS Status;

  /*
   * Create the vars with default value.
   * If we don't, forms won't be able to update.
   */

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"HypEnable",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdHypEnable, PcdGet32 (PcdHypEnable));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"HypLogMask",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdHypLogMask, PcdGet32 (PcdHypLogMask));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"HypWindowsDebugHook",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdHypWindowsDebugHook,
              PcdGet32 (PcdHypWindowsDebugHook));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"HypWin2000Mask",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdHypWin2000Mask, PcdGet32 (PcdHypWin2000Mask));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"CpuClock",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdCpuClock, PcdGet32 (PcdCpuClock));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"SdIsArasan",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdSdIsArasan, PcdGet32 (PcdSdIsArasan));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"MmcDisableMulti",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdMmcDisableMulti, PcdGet32 (PcdMmcDisableMulti));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"MmcForce1Bit",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdMmcForce1Bit, PcdGet32 (PcdMmcForce1Bit));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"MmcForceDefaultSpeed",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdMmcForceDefaultSpeed, PcdGet32 (PcdMmcForceDefaultSpeed));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"MmcSdDefaultSpeedMHz",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdMmcSdDefaultSpeedMHz, PcdGet32 (PcdMmcSdDefaultSpeedMHz));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"MmcSdHighSpeedMHz",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdMmcSdHighSpeedMHz, PcdGet32 (PcdMmcSdHighSpeedMHz));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"DebugEnableJTAG",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdDebugEnableJTAG, PcdGet32 (PcdDebugEnableJTAG));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"DebugShowUEFIExit",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdDebugShowUEFIExit, PcdGet32 (PcdDebugShowUEFIExit));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"DisplayEnableVModes",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdDisplayEnableVModes, PcdGet32 (PcdDisplayEnableVModes));
  }

  Size = sizeof (UINT32);
  Status = gRT->GetVariable(L"DisplayEnableSShot",
                            &gConfigDxeFormSetGuid,
                            NULL,  &Size, &Var32);
  if (EFI_ERROR (Status)) {
    PcdSet32 (PcdDisplayEnableSShot, PcdGet32 (PcdDisplayEnableSShot));
  }

  return EFI_SUCCESS;
}
  

STATIC VOID
ApplyVariables (
  VOID
  )
{
  UINTN Gpio34Group;
  UINTN Gpio48Group;
  EFI_STATUS Status;
  UINT32 CpuClock = PcdGet32 (PcdCpuClock);
  UINT32 Rate = 0;

  if (CpuClock != 0) {
    if (CpuClock == 2) {
      /*
       * Maximum: 1.2GHz on RPi 3, 1.4GHz on RPi 3B+, unless
       * overridden with arm_freq=xxx in config.txt.
       */
      Status = mFwProtocol->GetMaxClockRate(RPI_FW_CLOCK_RATE_ARM, &Rate);
      if (Status != EFI_SUCCESS) {
        DEBUG((EFI_D_ERROR, "Couldn't get the max CPU speed, leaving as is: %r\n",
               Status));
      }
    } else {
      Rate = 600 * 1000000;
    }
  }

  if (Rate != 0) {
    DEBUG((EFI_D_INFO, "Setting CPU speed to %uHz\n", Rate));
    Status = mFwProtocol->SetClockRate(RPI_FW_CLOCK_RATE_ARM, Rate);
    if (Status != EFI_SUCCESS) {
      DEBUG((EFI_D_ERROR, "Couldn't set the CPU speed: %r\n",
             Status));
    }
  }

  Status = mFwProtocol->GetClockRate(RPI_FW_CLOCK_RATE_ARM, &Rate);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "Couldn't get the CPU speed: %r\n",
           Status));
  } else {
    DEBUG((EFI_D_INFO, "Current CPU speed is %uHz\n", Rate));
  }

  /*
   * Switching two groups around, so disable both first.
   *
   * No, I've not seen a problem, but having a group be
   * routed to two sets of pins seems like asking for trouble.
   */
  GpioPinFuncSet(34, GPIO_FSEL_INPUT);
  GpioPinFuncSet(35, GPIO_FSEL_INPUT);
  GpioPinFuncSet(36, GPIO_FSEL_INPUT);
  GpioPinFuncSet(37, GPIO_FSEL_INPUT);
  GpioPinFuncSet(38, GPIO_FSEL_INPUT);
  GpioPinFuncSet(39, GPIO_FSEL_INPUT);
  GpioPinFuncSet(48, GPIO_FSEL_INPUT);
  GpioPinFuncSet(49, GPIO_FSEL_INPUT);
  GpioPinFuncSet(50, GPIO_FSEL_INPUT);
  GpioPinFuncSet(51, GPIO_FSEL_INPUT);
  GpioPinFuncSet(52, GPIO_FSEL_INPUT);
  GpioPinFuncSet(53, GPIO_FSEL_INPUT);
  if (PcdGet32 (PcdSdIsArasan)) {
    DEBUG((EFI_D_INFO, "Routing SD to Arasan\n"));
    Gpio48Group = GPIO_FSEL_ALT3;
    /*
     * Route SDIO to SdHost.
     */
    Gpio34Group = GPIO_FSEL_ALT0;
  } else {
    DEBUG((EFI_D_INFO, "Routing SD to SdHost\n"));
    Gpio48Group = GPIO_FSEL_ALT0;
    /*
     * Route SDIO to Arasan.
     */
    Gpio34Group = GPIO_FSEL_ALT3;
  }
  GpioPinFuncSet(34, Gpio34Group);
  GpioPinFuncSet(35, Gpio34Group);
  GpioPinFuncSet(36, Gpio34Group);
  GpioPinFuncSet(37, Gpio34Group);
  GpioPinFuncSet(38, Gpio34Group);
  GpioPinFuncSet(39, Gpio34Group);
  GpioPinFuncSet(48, Gpio48Group);
  GpioPinFuncSet(49, Gpio48Group);
  GpioPinFuncSet(50, Gpio48Group);
  GpioPinFuncSet(51, Gpio48Group);
  GpioPinFuncSet(52, Gpio48Group);
  GpioPinFuncSet(53, Gpio48Group);

  /*
   * JTAG pin    JTAG sig    GPIO      Mode    Header pin
   * 1           VREF        N/A               1
   * 3           nTRST       GPIO22    ALT4    15
   * 4           GND         N/A               9
   * 5           TDI         GPIO4     ALT5    7
   * 7           TMS         GPIO27    ALT4    13
   * 9           TCK         GPIO25    ALT4    22
   * 11          RTCK        GPIO23    ALT4    16
   * 13          TDO         GPIO24    ALT4    18
   */
  if (PcdGet32 (PcdDebugEnableJTAG)) {
    GpioPinFuncSet(22, GPIO_FSEL_ALT4);
    GpioPinFuncSet(4,  GPIO_FSEL_ALT5);
    GpioPinFuncSet(27, GPIO_FSEL_ALT4);
    GpioPinFuncSet(25, GPIO_FSEL_ALT4);
    GpioPinFuncSet(23, GPIO_FSEL_ALT4);
    GpioPinFuncSet(24, GPIO_FSEL_ALT4);
  } else {
    GpioPinFuncSet(22, GPIO_FSEL_INPUT);
    GpioPinFuncSet(4,  GPIO_FSEL_INPUT);
    GpioPinFuncSet(27, GPIO_FSEL_INPUT);
    GpioPinFuncSet(25, GPIO_FSEL_INPUT);
    GpioPinFuncSet(23, GPIO_FSEL_INPUT);
    GpioPinFuncSet(24, GPIO_FSEL_INPUT);
  }
}


EFI_STATUS
EFIAPI
ConfigInitialize(
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (&gRaspberryPiFirmwareProtocolGuid,
                                NULL, (VOID **)&mFwProtocol);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SetupVariables();
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "Couldn't not setup NV vars: %r\n",
           Status));
  }

  ApplyVariables();
  Status = gBS->InstallProtocolInterface (&ImageHandle,
                                          &gRaspberryPiConfigAppliedProtocolGuid,
                                          EFI_NATIVE_INTERFACE,
                                          NULL);
  ASSERT_EFI_ERROR (Status);

  Status = InstallHiiPages();
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "Couldn't install ConfigDxe configuration pages: %r\n",
           Status));
  }

  return EFI_SUCCESS;
}

