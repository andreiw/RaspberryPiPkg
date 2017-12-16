//
// Copyright (c) 2016, Linaro, Ltd. All rights reserved.
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//

#ifndef __RASPBERRY_PI_FIRMWARE_PROTOCOL_H__
#define __RASPBERRY_PI_FIRMWARE_PROTOCOL_H__

#define RASPBERRY_PI_FIRMWARE_PROTOL_GUID \
  { 0x0ACA9535, 0x7AD0, 0x4286, { 0xB0, 0x2E, 0x87, 0xFA, 0x7E, 0x2A, 0x57, 0x11 } }

typedef
EFI_STATUS
(EFIAPI *SET_POWER_STATE) (
  IN  UINT32    DeviceId,
  IN  BOOLEAN   PowerState,
  IN  BOOLEAN   Wait
  );

typedef
EFI_STATUS
(EFIAPI *GET_MAC_ADDRESS) (
  OUT UINT8     MacAddress[6]
  );

typedef
EFI_STATUS
(EFIAPI *GET_COMMAND_LINE) (
  IN  UINTN     BufferSize,
  OUT CHAR8     CommandLine[]
  );

typedef
EFI_STATUS
(EFIAPI *GET_CLOCK_RATE) (
  IN  UINT32    ClockId,
  OUT UINT32    *ClockRate
  );

typedef
EFI_STATUS
(EFIAPI *GET_FB) (
  IN  UINT32 Width,
  IN  UINT32 Height,
  IN  UINT32 Depth,
  OUT EFI_PHYSICAL_ADDRESS *FbBase,
  OUT UINTN *FbSize,
  OUT UINTN *Pitch
  );

typedef
EFI_STATUS
(EFIAPI *GET_FB_SIZE) (
  OUT   UINT32 *Width,
  OUT   UINT32 *Height
  );

typedef
EFI_STATUS
(EFIAPI *FREE_FB) (
  VOID
  );

typedef
VOID
(EFIAPI *SET_LED) (
  BOOLEAN On
  );

typedef
EFI_STATUS
(EFIAPI *GET_SERIAL) (
  UINT64 *Serial
  );

typedef
EFI_STATUS
(EFIAPI *GET_ARM_MEM) (
  UINT32 *Base,
  UINT32 *Size
  );

typedef struct {
  SET_POWER_STATE   SetPowerState;
  GET_MAC_ADDRESS   GetMacAddress;
  GET_COMMAND_LINE  GetCommandLine;
  GET_CLOCK_RATE    GetClockRate;
  GET_CLOCK_RATE    GetMaxClockRate;
  GET_CLOCK_RATE    GetMinClockRate;
  GET_FB            GetFB;
  FREE_FB           FreeFB;
  GET_FB_SIZE       GetFBSize;
  SET_LED           SetLed;
  GET_SERIAL        GetSerial;
  GET_ARM_MEM       GetArmMem;
} RASPBERRY_PI_FIRMWARE_PROTOCOL;

extern EFI_GUID gRaspberryPiFirmwareProtocolGuid;

#endif
