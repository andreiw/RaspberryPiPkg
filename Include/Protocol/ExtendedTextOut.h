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

#ifndef __EXTENDED_TEXT_OUT_H__
#define __EXTENDED_TEXT_OUT_H__

#include <Protocol/SimpleTextOut.h>
#include <Protocol/GraphicsOutput.h>

#define EXTENDED_TEXT_OUTPUT_PROTOCOL_GUID \
  { \
    0x387477ff, 0xffc7, 0xffd2, {0x8e, 0x39, 0x0, 0xff, 0xc9, 0x69, 0x72, 0x3b } \
  }

typedef struct _EXTENDED_TEXT_OUTPUT_PROTOCOL EXTENDED_TEXT_OUTPUT_PROTOCOL;

struct _EXTENDED_TEXT_OUTPUT_PROTOCOL {
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *TextOut;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput;
  BOOLEAN                         AutoWrap;
};

extern EFI_GUID gExtendedTextOutputProtocolGuid;

#endif /* __EXTENDED_TEXT_OUT__ */
