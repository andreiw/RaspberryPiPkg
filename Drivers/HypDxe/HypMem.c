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

#include <HypDxe.h>
#include <Protocol/LoadedImage.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC UINT8 __attribute__((__aligned__(EFI_PAGE_SIZE))) mPages[20 * EFI_PAGE_SIZE];

STATIC UINT8 *mNextPage = mPages;
STATIC UINT8 *mPageEnd = mPages + sizeof(mPages);

STATIC EFI_PHYSICAL_ADDRESS mHypFirst;
STATIC EFI_PHYSICAL_ADDRESS mHypLast;


EFI_STATUS
HypMemInit(
  IN  EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS Status;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;

  Status = gBS->HandleProtocol(
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID **) &LoadedImage);
  ASSERT_EFI_ERROR (Status);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  mHypFirst = (EFI_PHYSICAL_ADDRESS)
    LoadedImage->ImageBase;
  mHypLast = mHypFirst +
    LoadedImage->ImageSize - 1;
  DEBUG((EFI_D_INFO, "HypDxe at 0x%lx-0x%lx\n",
         mHypFirst, mHypLast));
  return EFI_SUCCESS;
}


VOID *
HypMemAlloc(
  IN  UINTN Pages
  )
{
  UINT8 *P;
  UINTN Size;
  ASSERT (((UINTN) mNextPage & EFI_PAGE_MASK) == 0);

  P = mNextPage;
  Size = EFI_PAGE_SIZE * Pages;
  if ((P + Size > mPageEnd)) {
    DEBUG((EFI_D_ERROR, "Not enough pages\n"));
    return NULL;
  }

  mNextPage += Size;
  DEBUG((EFI_D_INFO, "Remaining pages: %u\n",
         (mPageEnd - mNextPage) / EFI_PAGE_SIZE));
  return P;
}


BOOLEAN
HypMemIsHyp2M(
  IN  EFI_PHYSICAL_ADDRESS A
  )
{
  EFI_PHYSICAL_ADDRESS HypFirst2MB = A_DOWN(mHypFirst, SIZE_2MB);
  EFI_PHYSICAL_ADDRESS HypAfterLast2MB = A_UP(mHypLast, SIZE_2MB);

  if (A >= HypFirst2MB &&
      A < HypAfterLast2MB) {
    return TRUE;
  }

  return FALSE;
}


BOOLEAN
HypMemIsHypAddr(
  IN  EFI_PHYSICAL_ADDRESS A
  )
{
  if (A >= mHypFirst && A <= mHypLast) {
    return TRUE;
  }

  return FALSE;
}
