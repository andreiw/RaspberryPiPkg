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
#include "ArmDefs.h"
#include <Library/ArmSmcLib.h>

#define HVC_LOG_CALL_MASK        0xff00
#define HVC_LOG_CALL_ARG_MASK    0x7f
#define HVC_LOG_CALL_ARG_NL      0x80
#define HVC_LOG_CALL_ARG_STR     0x10
#define HVC_LOG_CALL_ARG_HEX     0x11
#define HVC_LOG_CALL_ARG_UDEC    0x12
#define HVC_LOG_CALL_ARG_SDEC    0x13


VOID
HypHVCProcess(
  IN  EFI_SYSTEM_CONTEXT_AARCH64 *SystemContext
  )
{
  UINTN HCall = ESR_2_ISS(SystemContext->ESR);
  if ((HCall & HVC_LOG_CALL_MASK) != 0) {
    UINTN PrintArg;
    CHAR8 *PFmt = NULL;
    static CHAR8 *PrintHex = "%lx";
    static CHAR8 *PrintChar = "%c";
    static CHAR8 *PrintUDec = "%lu";
    static CHAR8 *PrintSDec = "%ld";
    static CHAR8 *PrintStr = "%s";

    BOOLEAN NewLine = HCall & HVC_LOG_CALL_ARG_NL;
    UINT8 Arg = HCall & HVC_LOG_CALL_ARG_MASK;
    if (Arg >= 0 &&
        Arg <= 0xf) {
      /*
       * Hex digits.
       */
      PrintArg = Arg;
      PFmt = PrintHex;
    } else if (Arg >= 0x20 &&
               Arg < 0x7e) {
      /*
       * ASCII printables.
       */
      PrintArg = Arg;
      PFmt = PrintChar;
    } else if (Arg == HVC_LOG_CALL_ARG_STR) {
      PFmt = PrintStr;
      PrintArg = SystemContext->X0;
    } else if (Arg == HVC_LOG_CALL_ARG_HEX) {
      PFmt = PrintHex;
      PrintArg = SystemContext->X0;
    } else if (Arg == HVC_LOG_CALL_ARG_UDEC) {
      PFmt = PrintUDec;
      PrintArg = SystemContext->X0;
    } else if (Arg == HVC_LOG_CALL_ARG_SDEC) {
      PFmt = PrintSDec;
      PrintArg = SystemContext->X0;
    }

    if (PFmt != NULL) {
      HypLog(HLOG_VM, PFmt, PrintArg);
      if (NewLine != 0) {
        HypLog(HLOG_VM, "\n");
      }
    }
  }

  HLOG((HLOG_ERROR, "0x%lx: Ignoring Unknown HVC(%u) %x %x %x %x\n",
        SystemContext->ELR, ESR_2_ISS(SystemContext->ESR),
        SystemContext->X0, SystemContext->X1,
        SystemContext->X2, SystemContext->X3));
}
