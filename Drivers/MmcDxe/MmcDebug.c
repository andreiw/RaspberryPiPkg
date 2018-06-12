/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
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

#include "Mmc.h"

#if !defined(MDEPKG_NDEBUG)
CONST CHAR8* mStrUnit[] = { "100kbit/s", "1Mbit/s", "10Mbit/s", "100MBit/s",
                            "Unknown", "Unknown", "Unknown", "Unknown" };
CONST CHAR8* mStrValue[] = { "UNDEF", "1.0", "1.2", "1.3", "1.5", "2.0", "2.5", "3.0", "3.5", "4.0", "4.5", "5.0",
                             "Unknown", "Unknown", "Unknown", "Unknown" };
#endif

VOID
PrintCID(
    IN CID* Cid
    )
{
    DEBUG((EFI_D_BLKIO, "- PrintCID\n"));
    DEBUG((EFI_D_BLKIO,
        "\t- Manufacturing date: %d/%d\n",
        (UINT32)Cid->MDT & 0xF,
        (UINT32)((Cid->MDT >> 4) & 0xFF)));
    DEBUG((EFI_D_BLKIO, "\t- Product serial number: 0x%X\n", (UINT32)Cid->PSN));
    DEBUG((EFI_D_BLKIO, "\t- Product revision: %d\n", (UINT32)Cid->PRV));

    DEBUG((
        EFI_D_BLKIO,
        "\t- Product name: %c%c%c%c%c\n",
        Cid->PNM[0],
        Cid->PNM[1],
        Cid->PNM[2],
        Cid->PNM[3],
        Cid->PNM[4]));

    DEBUG((EFI_D_BLKIO, "\t- OEM ID: 0x%x\n", (UINT32)Cid->OID));
}

VOID
PrintCSD(
    IN CSD* Csd
    )
{
    if (Csd->CSD_STRUCTURE == 0) {
        DEBUG((EFI_D_BLKIO, "- PrintCSD Version 1.01-1.10/Version 2.00/Standard Capacity\n"));
    } else if (Csd->CSD_STRUCTURE == 1) {
        DEBUG((EFI_D_BLKIO, "- PrintCSD Version 2.00/High Capacity\n"));
    } else {
        DEBUG((EFI_D_BLKIO, "- PrintCSD Version Higher than v3.3\n"));
    }

    DEBUG((EFI_D_BLKIO, "\t- Supported card command class: 0x%X\n", (UINT32)Csd->CCC));

    DEBUG((
        EFI_D_BLKIO, "\t- Speed: %a %a (TRAN_SPEED:%x)\n",
        mStrValue[(Csd->TRAN_SPEED >> 3) & 0xF],
        mStrUnit[Csd->TRAN_SPEED & 7],
        (UINT32)Csd->TRAN_SPEED));

    DEBUG((
        EFI_D_BLKIO, "\t- Maximum Read Data Block: %d (READ_BL_LEN:%x)\n",
        (UINT32)(1 << Csd->READ_BL_LEN),
        (UINT32)Csd->READ_BL_LEN));

    DEBUG((
        EFI_D_BLKIO, "\t- Maximum Write Data Block: %d (WRITE_BL_LEN:%x)\n",
        (UINT32)(1 << Csd->WRITE_BL_LEN),
        (UINT32)Csd->WRITE_BL_LEN));

    if (!Csd->FILE_FORMAT_GRP) {
        if (Csd->FILE_FORMAT == 0) {
            DEBUG((EFI_D_BLKIO, "\t- Format (0): Hard disk-like file system with partition table\n"));
        } else if (Csd->FILE_FORMAT == 1) {
            DEBUG((EFI_D_BLKIO, "\t- Format (1): DOS FAT (floppy-like) with boot sector only (no partition table)\n"));
        } else if (Csd->FILE_FORMAT == 2) {
            DEBUG((EFI_D_BLKIO, "\t- Format (2): Universal File Format\n"));
        } else {
            DEBUG((EFI_D_BLKIO, "\t- Format (3): Others/Unknown\n"));
        }
    } else {
        DEBUG((EFI_D_BLKIO, "\t- Format: Reserved\n"));
    }
}

VOID
PrintRCA(
    IN UINT32 Rca
    )
{
    DEBUG((EFI_D_BLKIO, "- PrintRCA: 0x%X\n", Rca));
    DEBUG((EFI_D_BLKIO, "\t- Status: 0x%X\n", Rca & 0xFFFF));
    DEBUG((EFI_D_BLKIO, "\t- RCA: 0x%X\n", (Rca >> 16) & 0xFFFF));
}

VOID
PrintOCR(
    IN UINT32 Ocr
    )
{
    UINTN MinV;
    UINTN MaxV;
    UINTN Volts;
    UINTN Loop;

    MinV = 36;  // 3.6
    MaxV = 20;  // 2.0
    Volts = 20;  // 2.0

    // The MMC register bits [23:8] indicate the working range of the card
    for (Loop = 8; Loop < 24; Loop++) {
        if (Ocr & (1 << Loop)) {
            if (MinV > Volts) {
                MinV = Volts;
            }
            if (MaxV < Volts) {
                MaxV = Volts + 1;
            }
        }
        Volts++;
    }

    DEBUG((EFI_D_BLKIO, "- PrintOCR Ocr (0x%X)\n", Ocr));
    DEBUG((EFI_D_BLKIO, "\t- Card operating voltage: %d.%d to %d.%d\n", MinV / 10, MinV % 10, MaxV / 10, MaxV % 10));
    if (((Ocr >> 29) & 3) == 0) {
        DEBUG((EFI_D_BLKIO, "\t- AccessMode: Byte Mode\n"));
    } else {
        DEBUG((EFI_D_BLKIO, "\t- AccessMode: Block Mode (0x%X)\n", ((Ocr >> 29) & 3)));
    }

    if (Ocr & MMC_OCR_POWERUP) {
        DEBUG((EFI_D_BLKIO, "\t- PowerUp\n"));
    } else {
        DEBUG((EFI_D_BLKIO, "\t- Voltage Not Supported\n"));
    }
}

VOID
PrintResponseR1(
    IN  UINT32 Response
    )
{
    DEBUG((EFI_D_BLKIO, "Response: 0x%X\n", Response));
    if (Response & MMC_R0_READY_FOR_DATA) {
        DEBUG((EFI_D_BLKIO, "\t- READY_FOR_DATA\n"));
    }

    switch ((Response >> 9) & 0xF) {
    case 0:
        DEBUG((EFI_D_BLKIO, "\t- State: Idle\n"));
        break;
    case 1:
        DEBUG((EFI_D_BLKIO, "\t- State: Ready\n"));
        break;
    case 2:
        DEBUG((EFI_D_BLKIO, "\t- State: Ident\n"));
        break;
    case 3:
        DEBUG((EFI_D_BLKIO, "\t- State: StandBy\n"));
        break;
    case 4:
        DEBUG((EFI_D_BLKIO, "\t- State: Tran\n"));
        break;
    case 5:
        DEBUG((EFI_D_BLKIO, "\t- State: Data\n"));
        break;
    case 6:
        DEBUG((EFI_D_BLKIO, "\t- State: Rcv\n"));
        break;
    case 7:
        DEBUG((EFI_D_BLKIO, "\t- State: Prg\n"));
        break;
    case 8:
        DEBUG((EFI_D_BLKIO, "\t- State: Dis\n"));
        break;
    default:
        DEBUG((EFI_D_BLKIO, "\t- State: Reserved\n"));
        break;
    }
}
