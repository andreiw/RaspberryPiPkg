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

#include "HypDxe.h"
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>

#define ESC                       27
#define BRIGHT_CONTROL_OFFSET     2
#define FOREGROUND_CONTROL_OFFSET 6
#define COL_BLACK                 30
#define COL_BLUE                  34
#define COL_GREEN                 32
#define COL_CYAN                  36
#define COL_RED                   31
#define COL_MAGENTA               35
#define COL_BROWN                 33
#define COL_LIGHTGRAY             37
#define BRIGHT(x)                 (x + 100)
#define IS_BRIGHT(x)              (x > 100)
#define DEBRIGHT(x)               (x - 100)
#define COL_DEFAULT               COL_LIGHTGRAY

STATIC CHAR8 mBuffer[EFI_PAGE_SIZE];
STATIC UINTN mBufferSize = EFI_PAGE_SIZE;
STATIC BOOLEAN mLogLock = SL_UNLOCKED;
STATIC UINT32 mLogMask = 0x0;


EFI_STATUS
HypLogInit(
  VOID
  )
{
  mLogMask = PcdGet32(PcdHypLogMask);
  return EFI_SUCCESS;
}


STATIC VOID
HypLogSetColors(
  IN  UINTN Color
  )
{
  UINTN BrightControl;
  UINTN ForegroundControl;
  CHAR8 mSetAttributeString[] = { ESC, '[', '0', 'm', ESC, '[', '4', '0', 'm', 0 };

  if (IS_BRIGHT(Color)) {
    BrightControl = 1;
    ForegroundControl = DEBRIGHT(Color);
  } else {
    BrightControl = 0;
    ForegroundControl = Color;
  }

  mSetAttributeString[BRIGHT_CONTROL_OFFSET] = '0' + BrightControl;
  mSetAttributeString[FOREGROUND_CONTROL_OFFSET + 0]  = '0' + (ForegroundControl / 10);
  mSetAttributeString[FOREGROUND_CONTROL_OFFSET + 1]  = '0' + (ForegroundControl % 10);

  SerialPortWrite (U8P(mSetAttributeString),
                   sizeof(mSetAttributeString));  
}


STATIC VOID
HypLogWrite(
  IN  CHAR8 *String,
  IN  UINTN Len
  )
{
  CHAR8 *NL = "\r\n";
  CHAR8 *End = String + Len;

  while (String < End) {
    if (*String == '\0') {
      break;
    }

    if (*String != '\n') {
      SerialPortWrite(U8P(String), 1);
    } else {
      SerialPortWrite(U8P(NL), 2);
    }

    String++;
  }
}


VOID
HypLog (
  IN  UINT32       ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST Marker;
  UINTN Printed;
  UINTN Color;

  if (ArmReadCurrentEL() != AARCH64_EL2 &&
      HypIsEnabled()) {
    /*
     * HypDxe is already protected from EL1. Cannot use own
     * logging facilities directly.
     *
     * FIXME: go through EL2.
     */
    return;
  }

  if (ErrorLevel != HLOG_ERROR &&
      (ErrorLevel & mLogMask) == 0) {
    return;
  }

  //
  // If Format is NULL, then ASSERT().
  //
  ASSERT (Format != NULL);

  SLock(&mLogLock);
  //
  // Convert the DEBUG() message to an ASCII String
  //
  VA_START (Marker, Format);
  Printed = AsciiVSPrint (mBuffer, mBufferSize, Format, Marker);
  VA_END (Marker);

  if (ErrorLevel == HLOG_ERROR) {
    Color = COL_RED;
  } else if (ErrorLevel == HLOG_INFO) {
    Color = BRIGHT(COL_GREEN);
  } else if (ErrorLevel == HLOG_VM) {
    Color = BRIGHT(COL_BLUE);
  } else {
    Color = COL_DEFAULT;
  }
  HypLogSetColors(Color);
  HypLogWrite (mBuffer, Printed);
  if (Color != COL_DEFAULT) {
    HypLogSetColors(COL_DEFAULT);
  }
  SUnlock(&mLogLock);
}


VOID
EFIAPI
HypAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
  UINTN Printed;

  SLock(&mLogLock);
  Printed = AsciiSPrint (mBuffer, mBufferSize, "ASSERT [%a] %a(%d): %a\n",
                         gEfiCallerBaseName, FileName, LineNumber,
                         Description);

  HypLogWrite (mBuffer, Printed);
  SUnlock(&mLogLock);

  CpuBreakpoint ();
}
