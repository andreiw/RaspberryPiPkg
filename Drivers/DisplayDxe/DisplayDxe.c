/** @file
*
*  Copyright (c), 2017, Andrei Warkentin <andrey.warkentin@gmail.com>
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
#include <Protocol/Cpu.h>

#define POS_TO_FB(posX, posY) ((UINT8 *)                                \
                               ((UINTN)This->Mode->FrameBufferBase +    \
                                (posY) * This->Mode->Info->PixelsPerScanLine * \
                                PI2_BYTES_PER_PIXEL +                   \
                                (posX) * PI2_BYTES_PER_PIXEL))
typedef struct {
  VENDOR_DEVICE_PATH DisplayDevicePath;
  EFI_DEVICE_PATH EndDevicePath;
} DISPLAY_DEVICE_PATH;

DISPLAY_DEVICE_PATH mDisplayDevicePath =
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof(VENDOR_DEVICE_PATH)),
          (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8),
        }
      },
      EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      {
        sizeof(EFI_DEVICE_PATH_PROTOCOL),
        0
      }
    }
  };

#define PI2_BITS_PER_PIXEL              (32)
#define PI2_BYTES_PER_PIXEL             (PI2_BITS_PER_PIXEL / 8)

STATIC
EFI_STATUS
EFIAPI
DisplayQueryMode(
                 IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
                 IN  UINT32                                ModeNumber,
                 OUT UINTN                                 *SizeOfInfo,
                 OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
                 );

STATIC
EFI_STATUS
EFIAPI
DisplaySetMode(
               IN  EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
               IN  UINT32                       ModeNumber
               );

STATIC
EFI_STATUS
EFIAPI
DisplayBlt(
           IN  EFI_GRAPHICS_OUTPUT_PROTOCOL            *This,
           IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL           *BltBuffer,   OPTIONAL
           IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION       BltOperation,
           IN  UINTN                                   SourceX,
           IN  UINTN                                   SourceY,
           IN  UINTN                                   DestinationX,
           IN  UINTN                                   DestinationY,
           IN  UINTN                                   Width,
           IN  UINTN                                   Height,
           IN  UINTN                                   Delta         OPTIONAL
           );

STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL mDisplay = {
  DisplayQueryMode,
  DisplaySetMode,
  DisplayBlt,
  NULL
};

STATIC
EFI_STATUS
EFIAPI
DisplayQueryMode(
                 IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
                 IN  UINT32                                ModeNumber,
                 OUT UINTN                                 *SizeOfInfo,
                 OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
                 )
{
  EFI_STATUS Status;

  Status = gBS->AllocatePool(
                             EfiBootServicesData,
                             sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),
                             (VOID **)Info
                             );
  ASSERT_EFI_ERROR(Status);

  *SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  (*Info)->Version = This->Mode->Info->Version;
  (*Info)->HorizontalResolution = This->Mode->Info->HorizontalResolution;
  (*Info)->VerticalResolution = This->Mode->Info->VerticalResolution;
  (*Info)->PixelFormat = This->Mode->Info->PixelFormat;
  (*Info)->PixelsPerScanLine = This->Mode->Info->PixelsPerScanLine;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DisplaySetMode(
               IN  EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
               IN  UINT32                       ModeNumber
               )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DisplayBlt(
           IN  EFI_GRAPHICS_OUTPUT_PROTOCOL      *This,
           IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *BltBuffer,   OPTIONAL
           IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
           IN  UINTN                             SourceX,
           IN  UINTN                             SourceY,
           IN  UINTN                             DestinationX,
           IN  UINTN                             DestinationY,
           IN  UINTN                             Width,
           IN  UINTN                             Height,
           IN  UINTN                             Delta         OPTIONAL
           )
{
  UINT8 *VidBuf, *BltBuf, *VidBuf1;
  UINTN i;

  switch(BltOperation) {
  case EfiBltVideoFill:
    BltBuf = (UINT8 *)BltBuffer;

    for (i = 0; i < Height; i++) {
      VidBuf = POS_TO_FB(DestinationX, DestinationY + i);

      SetMem32(VidBuf, Width * PI2_BYTES_PER_PIXEL, *(UINT32 *) BltBuf);
    }
    break;

  case EfiBltVideoToBltBuffer:
    if (Delta == 0) {
      Delta = Width * PI2_BYTES_PER_PIXEL;
    }

    for (i = 0; i < Height; i++) {
      VidBuf = POS_TO_FB(SourceX, SourceY + i);

      BltBuf = (UINT8 *)((UINTN)BltBuffer + (DestinationY + i) * Delta +
                         DestinationX * PI2_BYTES_PER_PIXEL);

      gBS->CopyMem((VOID *)BltBuf, (VOID *)VidBuf, PI2_BYTES_PER_PIXEL * Width);
    }
    break;

  case EfiBltBufferToVideo:
    if (Delta == 0) {
      Delta = Width * PI2_BYTES_PER_PIXEL;
    }

    for (i = 0; i < Height; i++) {
      VidBuf = POS_TO_FB(DestinationX, DestinationY + i);
      BltBuf = (UINT8 *)((UINTN) BltBuffer + (SourceY + i) * Delta +
                         SourceX * PI2_BYTES_PER_PIXEL);

      gBS->CopyMem((VOID *)VidBuf, (VOID *)BltBuf, Width * PI2_BYTES_PER_PIXEL);
    }
    break;

  case EfiBltVideoToVideo:
    for (i = 0; i < Height; i++) {
      VidBuf = POS_TO_FB(SourceX, SourceY + i);
      VidBuf1 = POS_TO_FB(DestinationX, DestinationY + i);

      gBS->CopyMem((VOID *)VidBuf1, (VOID *)VidBuf, Width * PI2_BYTES_PER_PIXEL);
    }
    break;

  default:
    ASSERT_EFI_ERROR(EFI_SUCCESS);
    break;
  }

  return EFI_SUCCESS;
}

/**
   Initialize the state information for the Display Dxe

   @param  ImageHandle   of the loaded driver
   @param  SystemTable   Pointer to the System Table

   @retval EFI_SUCCESS           Protocol registered
   @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
   @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
EFIAPI
DisplayDxeInitialize (
                      IN EFI_HANDLE         ImageHandle,
                      IN EFI_SYSTEM_TABLE   *SystemTable
                      )
{
  UINT32 Width;
  UINT32 Height;
  UINTN FbSize;
  UINTN FbPitch;
  EFI_STATUS Status;
  EFI_PHYSICAL_ADDRESS FbBase;
  EFI_HANDLE gUEFIDisplayHandle = NULL;
  EFI_GUID GraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  EFI_GUID DevicePathProtocolGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;
  STATIC RASPBERRY_PI_FIRMWARE_PROTOCOL *FwProtocol;
  STATIC EFI_CPU_ARCH_PROTOCOL *Cpu;

  Status = gBS->LocateProtocol (&gRaspberryPiFirmwareProtocolGuid, NULL,
                                (VOID **)&FwProtocol);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &Cpu);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (mDisplay.Mode == NULL){
    Status = gBS->AllocatePool(
                               EfiBootServicesData,
                               sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE),
                               (VOID **)&mDisplay.Mode
                               );
    ASSERT_EFI_ERROR(Status);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    ZeroMem(mDisplay.Mode,sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE));
  }

  if(mDisplay.Mode->Info == NULL){
    Status = gBS->AllocatePool(
                               EfiBootServicesData,
                               sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),
                               (VOID **)&mDisplay.Mode->Info
                               );
    ASSERT_EFI_ERROR(Status);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    ZeroMem(mDisplay.Mode->Info,sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
  }

  // Query the current display resolution from mailbox
  Status = FwProtocol->GetFBSize(&Width, &Height);
  if(EFI_ERROR(Status)) {
    return Status;
  }
  DEBUG((EFI_D_INIT, "Mailbox Display Size  %d x %d\n", Width, Height));
  if (Width < 800 || Height < 600) {
    DEBUG((EFI_D_ERROR, "Display too small or not connected\n"));
    return EFI_UNSUPPORTED;
  }

  Status = FwProtocol->GetFB(Width, Height, PI2_BITS_PER_PIXEL, &FbBase,
                              &FbSize, &FbPitch);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  DEBUG((EFI_D_INFO, "Framebuffer is %u bytes at %p\n", FbSize, FbBase));

  ASSERT (FbPitch != 0);
  ASSERT (FbBase != 0);
  ASSERT (FbSize != 0);

  /*
   * WT, because certain OS loaders access the frame buffer directly
   * and we don't want to see corruption due to missing WB cache
   * maintenance. Performance with WT is good.
   */
  Status = Cpu->SetMemoryAttributes(Cpu, FbBase, FbSize, EFI_MEMORY_WT);
  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "Couldn't set framebuffer attributes: %r\n", Status));
    return Status;
  }

  // Fill out mode information
  mDisplay.Mode->MaxMode = 1;
  mDisplay.Mode->Mode = 0;
  mDisplay.Mode->Info->Version = 0;

  // There is no way to communicate pitch back to OS. OS and even UEFI

  // expects a fully linear frame buffer. So the width should
  // be based on the frame buffer's pitch value. In some cases VC
  // firmware would allocate a frame buffer with some padding
  // presumeably to be 8 byte align.
  mDisplay.Mode->Info->HorizontalResolution = FbPitch / PI2_BYTES_PER_PIXEL;
  mDisplay.Mode->Info->VerticalResolution = Height;

  // NOTE: Windows REQUIRES BGR in 32 or 24 bit format.
  mDisplay.Mode->Info->PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
  mDisplay.Mode->Info->PixelsPerScanLine = FbPitch / PI2_BYTES_PER_PIXEL;;
  mDisplay.Mode->SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  mDisplay.Mode->FrameBufferBase = FbBase;
  mDisplay.Mode->FrameBufferSize = FbSize;

  Status = gBS->InstallMultipleProtocolInterfaces (
                                                   &gUEFIDisplayHandle,
                                                   &DevicePathProtocolGuid,
                                                   &mDisplayDevicePath,
                                                   &GraphicsOutputProtocolGuid,
                                                   &mDisplay,
                                                   NULL);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
