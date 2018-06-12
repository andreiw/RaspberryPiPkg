/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
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

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>

#include "Mmc.h"
#define MAX_RETRY_COUNT                     1000
#define MULTI_BLK_XFER_MAX_BLK_CNT          10000
// Perform Integer division DIVIDEND/DIVISOR and return the result rounded up or down
// to the nearest integer, where 3.5 and 3.75 are near 4, while 3.25 is near 3.
#define INT_DIV_ROUND(DIVIDEND, DIVISOR)    (((DIVIDEND) + ((DIVISOR) / 2)) / (DIVISOR))
#define MMCI0_BLOCKLEN                      512
#define MMCI0_TIMEOUT                       10000

// The high-performance counter frequency
UINT64 mHpcTicksPerSeconds = 0;

VOID
MmcBenchmarkBlockIo(
    IN EFI_BLOCK_IO_PROTOCOL  *This,
    IN UINTN                  Transfer,
    IN UINT32                 MediaId,
    IN UINT32                 BufferByteSize,
    IN UINT32                 Iterations
    );

EFI_STATUS
SdSwitchHighSpeedMode(
    IN  MMC_HOST_INSTANCE   *MmcHostInstance
    );

VOID
SortIoReadStatsByTotalTransferTime(
    IoReadStatsEntry* Table,
    UINT32 NumEntries
    )
{
    // Using the simple insertion sort
    UINT32 Idx;
    for (Idx = 1; Idx < NumEntries; ++Idx) {
        IoReadStatsEntry CurrEntry = Table[Idx];
        UINT32 J = Idx;
        while (J > 0 &&
               Table[J - 1].TotalTransferTimeUs < CurrEntry.TotalTransferTimeUs) {
            Table[J] = Table[J - 1];
            --J;
        }
        Table[J] = CurrEntry;
    }
}

EFI_STATUS
MmcNotifyState(
    IN MMC_HOST_INSTANCE *MmcHostInstance,
    IN MMC_STATE State
    )
{
    MmcHostInstance->State = State;
    return MmcHostInstance->MmcHost->NotifyState(MmcHostInstance->MmcHost, State);
}

EFI_STATUS
EFIAPI
MmcGetCardStatus(
    IN MMC_HOST_INSTANCE     *MmcHostInstance
    )
{
    EFI_STATUS              Status;
    UINT32                  Response[4];
    UINTN                   CmdArg;
    EFI_MMC_HOST_PROTOCOL   *MmcHost;

    Status = EFI_SUCCESS;
    MmcHost = MmcHostInstance->MmcHost;
    CmdArg = 0;

    if (MmcHost == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    if (MmcHostInstance->State != MmcHwInitializationState) {
        //Get the Status of the card.
        CmdArg = MmcHostInstance->CardInfo.RCA << 16;
        Status = MmcHost->SendCommand(MmcHost, MMC_CMD13, CmdArg);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "MmcDxe: MmcGetCardStatus(MMC_CMD13): Error and Status = %r\n", Status));
            return Status;
        }

        //Read Response
        MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_R1, Response);
        PrintResponseR1(Response[0]);
    }

    return Status;
}

EFI_STATUS
EFIAPI
MmcIdentificationMode(
    IN MMC_HOST_INSTANCE     *MmcHostInstance
    )
{
    EFI_STATUS              Status;
    UINT32                  Response[4];
    UINTN                   Timeout;
    UINTN                   CmdArg;
    BOOLEAN                 IsHCS;
    EFI_MMC_HOST_PROTOCOL   *MmcHost;

    MmcHost = MmcHostInstance->MmcHost;
    CmdArg = 0;
    IsHCS = FALSE;

    if (MmcHost == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    //Force MSCH into HWinit state before Identified, it really need.
    MmcHostInstance->State = MmcHwInitializationState;

    // We can get into this function if we restart the identification mode
    if (MmcHostInstance->State == MmcHwInitializationState) {
        // Initialize the MMC Host HW
        Status = MmcNotifyState(MmcHostInstance, MmcHwInitializationState);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "MmcDxe: MmcIdentificationMode() : Error MmcHwInitializationState\n"));
            return Status;
        }
    }

    Status = MmcHost->SendCommand(MmcHost, MMC_CMD0, 0);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: MmcIdentificationMode(MMC_CMD0): Error\n"));
        return Status;
    }

    Status = MmcNotifyState(MmcHostInstance, MmcIdleState);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: MmcIdentificationMode() : Error MmcIdleState\n"));
        return Status;
    }

    // Check which kind of card we are using. Ver2.00 or later SD Memory Card (PL180 is SD v1.1)
    CmdArg = (0x0UL << 12 | BIT8 | 0xCEUL << 0);
    Status = MmcHost->SendCommand(MmcHost, MMC_CMD8, CmdArg);
    if (Status == EFI_SUCCESS) {
        DEBUG((DEBUG_INIT, "MmcDxe: Card is SD2.0 compliant\n"));
        IsHCS = TRUE;
        MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_R7, Response);
        PrintResponseR1(Response[0]);
        //check if it is valid response
        if (Response[0] != CmdArg) {
            DEBUG((EFI_D_ERROR, "MmcDxe: MmcIdentificationMode(): The Card is not usable\n"));
            return EFI_UNSUPPORTED;
        }
    } else {
        DEBUG((DEBUG_INIT, "MmcDxe: Not SD2.0 Card\n"));
    }

    // We need to wait for the MMC or SD card is ready => (gCardInfo.OCRData.PowerUp == 1)
    Timeout = MAX_RETRY_COUNT;
    while (Timeout > 0) {
        // SD Card or MMC Card ? CMD55 indicates to the card that the next
        // command is an application specific command
        Status = MmcHost->SendCommand(MmcHost, MMC_CMD55, 0);
        if (!EFI_ERROR(Status)) {
            DEBUG((EFI_D_INFO, "MmcDxe: Card should be SD\n"));
            if (IsHCS) {
                MmcHostInstance->CardInfo.CardType = SD_CARD_2_SDSC;
            } else {
                MmcHostInstance->CardInfo.CardType = SD_CARD;
            }

            // Note: The first time CmdArg will be zero
            CmdArg = ((UINTN *)&(MmcHostInstance->CardInfo.OCRData))[0];
            if (IsHCS) {
                CmdArg |= BIT30;
            }
            Status = MmcHost->SendCommand(MmcHost, MMC_ACMD41, CmdArg);
            if (!EFI_ERROR(Status)) {
                MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_OCR, Response);
                ((UINT32 *)&(MmcHostInstance->CardInfo.OCRData))[0] = Response[0];
            }
        } else {
            DEBUG((EFI_D_INFO, "MmcDxe: Card should be MMC\n"));
            MmcHostInstance->CardInfo.CardType = MMC_CARD;

            Status = MmcHost->SendCommand(MmcHost, MMC_CMD1, 0x40800000);
            if (!EFI_ERROR(Status)) {
                MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_OCR, Response);
                ((UINT32 *)&(MmcHostInstance->CardInfo.OCRData))[0] = Response[0];
            }
        }

        if (!EFI_ERROR(Status)) {
            if (!MmcHostInstance->CardInfo.OCRData.PowerUp) {
                MicroSecondDelay(1);
                Timeout--;
            } else {
                if ((MmcHostInstance->CardInfo.CardType == SD_CARD_2_SDSC) &&
                    (MmcHostInstance->CardInfo.OCRData.AccessMode & BIT1)) {
                    MmcHostInstance->CardInfo.CardType = SD_CARD_2_SDHC;
                    DEBUG((EFI_D_INIT, "MmcDxe: Card is SDHC High Capacity SDCard\n"));
                }
                break;  // The MMC/SD card is ready. Continue the Identification Mode
            }
        } else {
            MicroSecondDelay(1);
            Timeout--;
        }
    }

    if (Timeout == 0) {
        DEBUG((EFI_D_ERROR, "MmcDxe: MmcIdentificationMode(): No Card\n"));
        return EFI_NO_MEDIA;
    } else {
        PrintOCR(Response[0]);
    }

    Status = MmcNotifyState(MmcHostInstance, MmcReadyState);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: MmcIdentificationMode() : Error MmcReadyState\n"));
        return Status;
    }

    Status = MmcHost->SendCommand(MmcHost, MMC_CMD2, 0);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: MmcIdentificationMode(MMC_CMD2): Error\n"));
        return Status;
    }
    MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_CID, Response);
    PrintCID((CID*)Response);

    Status = MmcNotifyState(MmcHostInstance, MmcIdentificationState);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: MmcIdentificationMode() : Error MmcIdentificationState\n"));
        return Status;
    }

    //
    // Note, SD specifications say that "if the command execution causes a state change, it
    // will be visible to the host in the response to the next command"
    // The status returned for this CMD3 will be 2 - identification
    //
    CmdArg = 1;
    //CMD3 with RCA as argument, should be lsr 16
    Status = MmcHost->SendCommand(MmcHost, MMC_CMD3, CmdArg << 16);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: MmcIdentificationMode(MMC_CMD3): Error\n"));
        return Status;
    }

    MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_RCA, Response);
    PrintRCA(Response[0]);

    // For MMC card, RCA is assigned by CMD3 while CMD3 dumps the RCA for SD card
    if (MmcHostInstance->CardInfo.CardType != MMC_CARD) {
        MmcHostInstance->CardInfo.RCA = Response[0] >> 16;
    } else {
        MmcHostInstance->CardInfo.RCA = CmdArg;
    }

    Status = MmcNotifyState(MmcHostInstance, MmcStandByState);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: MmcIdentificationMode(): Error MmcStandByState\n"));
        return Status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS InitializeMmcDevice(
    IN  MMC_HOST_INSTANCE   *MmcHostInstance
    )
{
    UINT32                  Response[4];
    EFI_STATUS              Status;
    UINT64                  DeviceSize, NumBlocks, CardSizeBytes, CardSizeGB;
    UINT32                  CmdArg;
    EFI_MMC_HOST_PROTOCOL   *MmcHost;
    UINT32                  BlockCount;
    UINT32                  ECSD[128];
    
#if MMC_COLLECT_STATISTICS
    UINT64                  InitializationStartTime = GetPerformanceCounter();
#endif // MMC_COLLECT_STATISTICS

    BlockCount = 1;
    MmcHost = MmcHostInstance->MmcHost;

    MmcIdentificationMode(MmcHostInstance);
    // Card should be in StadnBy state when returning back

    // Read Card Specific Data (CSD)
    CmdArg = MmcHostInstance->CardInfo.RCA << 16;
    Status = MmcHost->SendCommand(MmcHost, MMC_CMD9, CmdArg);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: InitializeMmcDevice(MMC_CMD9): Error, Status=%r\n", Status));
        return Status;
    }

    MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_CSD, Response);

    PrintCSD((CSD*)Response);

    if (MmcHostInstance->CardInfo.CardType == SD_CARD_2_SDHC) {
        CSD_2* Csd2 = (CSD_2*)Response;
        DeviceSize = Csd2 ->C_SIZE;
        CardSizeBytes = (UINT64)(DeviceSize + 1) * 512llu * 1024llu;
    } else {
        CSD* Csd = (CSD*)Response;
        DeviceSize = ((Csd->C_SIZEHigh10 << 2) | Csd->C_SIZELow2);
        UINT32 BLOCK_LEN = 1 << Csd->READ_BL_LEN;
        UINT32 MULT = 1 << (Csd->C_SIZE_MULT + 2);
        UINT32 BLOCKNR = (DeviceSize + 1) * MULT;
        CardSizeBytes = BLOCKNR * BLOCK_LEN;
    }

    NumBlocks = (CardSizeBytes / MMCI0_BLOCKLEN);
    CardSizeGB = INT_DIV_ROUND(CardSizeBytes, (UINT64)(1024llu * 1024llu * 1024llu));
    DEBUG((
        EFI_D_INIT,
        "MmcDxe: CardSize: %ldGB, NumBlocks: %ld assuming BlockSize: %d\n",
        (UINT64)CardSizeGB,
        (UINT64)NumBlocks,
        MMCI0_BLOCKLEN));

    MmcHostInstance->BlockIo.Media->LastBlock = (NumBlocks - 1);
    MmcHostInstance->BlockIo.Media->BlockSize = MMCI0_BLOCKLEN;
    MmcHostInstance->BlockIo.Media->ReadOnly = MmcHost->IsReadOnly(MmcHost);
    MmcHostInstance->BlockIo.Media->MediaPresent = TRUE;
    MmcHostInstance->BlockIo.Media->MediaId++;

    CmdArg = MmcHostInstance->CardInfo.RCA << 16;
    Status = MmcHost->SendCommand(MmcHost, MMC_CMD7, CmdArg);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: InitializeMmcDevice(MMC_CMD7): Error and Status = %r\n", Status));
        return Status;
    }

    Status = MmcNotifyState(MmcHostInstance, MmcTransferState);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: InitializeMmcDevice(): Error MmcTransferState\n"));
        return Status;
    }

    if (MmcHostInstance->CardInfo.CardType == MMC_CARD) {
        // Fetch ECSD
        Status = MmcHost->SendCommand(MmcHost, MMC_CMD8, CmdArg);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "MmcDxe: InitializeMmcDevice(): ECSD fetch error, Status=%r.\n", Status));
        }

        Status = MmcHost->ReadBlockData(MmcHost, 0, 512, ECSD);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "MmcDxe: InitializeMmcDevice(): ECSD read error, Status=%r.\n", Status));
            return Status;
        }
        MmcHostInstance->BlockIo.Media->LastBlock = ECSD[53] - 1;
        MmcHostInstance->BlockIo.Media->BlockSize = 512;
        DEBUG((
            EFI_D_BLKIO,
            "MmcDxe: Emmc NumBlocks: %ld, BlockSize: 512\n",
            MmcHostInstance->BlockIo.Media->LastBlock));
        DEBUG((
            EFI_D_BLKIO,
            "MmcDxe: Emmc capability = %ld Bytes\n",
            (MmcHostInstance->BlockIo.Media->LastBlock) << 9));
    }

    // Set Block Length
    Status = MmcHost->SendCommand(MmcHost, MMC_CMD16, MmcHostInstance->BlockIo.Media->BlockSize);
    if (EFI_ERROR(Status)) {
        DEBUG((
            EFI_D_ERROR,
            "MmcDxe: InitializeMmcDevice(MMC_CMD16): Error MmcHostInstance->BlockIo.Media->BlockSize: %d and Error = %r\n",
            MmcHostInstance->BlockIo.Media->BlockSize, Status));
        return Status;
    }

    // Block Count (not used). Could return an error for SD card
    if (MmcHostInstance->CardInfo.CardType == MMC_CARD) {
        MmcHost->SendCommand(MmcHost, MMC_CMD23, BlockCount);
    }

    // SD2.0 specs added the HighSpeed access mode support for both
    // SDSC and SDHC cards
    if (MmcHostInstance->CardInfo.CardType >= SD_CARD_2_SDSC) {
        Status = SdSwitchHighSpeedMode(MmcHostInstance);
        if (EFI_ERROR(Status)) {
            DEBUG((
                EFI_D_ERROR,
                "MmcDxe: InitializeMmcDevice(): Failed to switch SDCard to HighSpeed mode, Status = %r\n",
                Status));
        } else {
            DEBUG((EFI_D_INIT, "MmcDxe: Switched SDCard to 25MB/s HighSpeed mode\n", Status));
        }
    }

    mHpcTicksPerSeconds = GetPerformanceCounterProperties(NULL, NULL);
    ASSERT(mHpcTicksPerSeconds != 0);

#if MMC_COLLECT_STATISTICS
    UINT32 ElapsedTimeMs =
        (UINT32)(((GetPerformanceCounter() - InitializationStartTime) * 1000UL) / mHpcTicksPerSeconds);
    DEBUG((
        EFI_D_BLKIO,
        "MmcDxe: Initialization complete in %dms\n",
        ElapsedTimeMs));
#endif // MMC_COLLECT_STATISTICS

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MmcReset(
    IN EFI_BLOCK_IO_PROTOCOL    *This,
    IN BOOLEAN                  ExtendedVerification
    )
{
    MMC_HOST_INSTANCE       *MmcHostInstance;

    MmcHostInstance = MMC_HOST_INSTANCE_FROM_BLOCK_IO_THIS(This);

    if (MmcHostInstance->MmcHost == NULL) {
        // Nothing to do
        return EFI_SUCCESS;
    }

    // If a card is not present then clear all media settings
    if (!MmcHostInstance->MmcHost->IsCardPresent(MmcHostInstance->MmcHost)) {
        MmcHostInstance->BlockIo.Media->MediaPresent = FALSE;
        MmcHostInstance->BlockIo.Media->LastBlock = 0;
        MmcHostInstance->BlockIo.Media->BlockSize = 512;  // Should be zero but there is a bug in DiskIo
        MmcHostInstance->BlockIo.Media->ReadOnly = FALSE;

        // Indicate that the driver requires initialization
        MmcHostInstance->State = MmcHwInitializationState;

        return EFI_SUCCESS;
    }

    // Implement me. Either send a CMD0 (could not work for some MMC host) or just turn off/turn
    //      on power and restart Identification mode
    return EFI_SUCCESS;
}

EFI_STATUS
MmcDetectCard(
    EFI_MMC_HOST_PROTOCOL     *MmcHost
    )
{
    if (!MmcHost->IsCardPresent(MmcHost)) {
        return EFI_NO_MEDIA;
    } else {
        return EFI_SUCCESS;
    }
}

EFI_STATUS
MmcStopTransmission(
    EFI_MMC_HOST_PROTOCOL     *MmcHost
    )
{
    EFI_STATUS              Status;
    UINT32                  Response[4];
    // Command 12 - Stop transmission (ends read or write)
    // Normally only needed for streaming transfers or after error.
    Status = MmcHost->SendCommand(MmcHost, MMC_CMD12, 0);
    if (!EFI_ERROR(Status)) {
        MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_R1b, Response);
    }
    return Status;
}

EFI_STATUS
MmcIoBlocks(
    IN EFI_BLOCK_IO_PROTOCOL    *This,
    IN UINTN                    Transfer,
    IN UINT32                   MediaId,
    IN EFI_LBA                  Lba,
    IN UINTN                    BufferSize,
    OUT VOID                    *Buffer
    )
{
    UINT32                  Response[4];
    EFI_STATUS              Status;
    UINTN                   CmdArg;
    INTN                    Timeout;
    UINTN                   Cmd;
    MMC_HOST_INSTANCE       *MmcHostInstance;
    EFI_MMC_HOST_PROTOCOL   *MmcHost;
    UINTN                   BytesRemainingToBeTransfered;
    UINTN                   BlockCount;
    UINTN                   CurrentBlockNum;

    DEBUG((
        DEBUG_BLKIO,
        "MmcDxe: MmcIoBlocks(%c, 0x%lx, 0x%xB)\n",
        (Transfer == MMC_IOBLOCKS_WRITE) ? 'W' : 'R',
        Lba,
        (UINT32)BufferSize));

    BlockCount = 1;
    MmcHostInstance = MMC_HOST_INSTANCE_FROM_BLOCK_IO_THIS(This);
    ASSERT(MmcHostInstance != NULL);
    MmcHost = MmcHostInstance->MmcHost;
    ASSERT(MmcHost);

    if (This->Media->MediaId != MediaId) {
        return EFI_MEDIA_CHANGED;
    }

    if ((MmcHost == NULL) || (Buffer == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    // Check if a Card is Present
    if (!MmcHostInstance->BlockIo.Media->MediaPresent) {
        return EFI_NO_MEDIA;
    }

    // All blocks must be within the device
    if ((Lba + (BufferSize / This->Media->BlockSize)) > (This->Media->LastBlock + 1)) {
        return EFI_INVALID_PARAMETER;
    }

    if ((Transfer == MMC_IOBLOCKS_WRITE) && (This->Media->ReadOnly == TRUE)) {
        return EFI_WRITE_PROTECTED;
    }

    // Reading 0 Byte is valid
    if (BufferSize == 0) {
        return EFI_SUCCESS;
    }

    // The buffer size must be an exact multiple of the block size
    if ((BufferSize % This->Media->BlockSize) != 0) {
        return EFI_BAD_BUFFER_SIZE;
    }

    // Check the alignment
    if ((This->Media->IoAlign > 2) && (((UINTN)Buffer & (This->Media->IoAlign - 1)) != 0)) {
        return EFI_INVALID_PARAMETER;
    }

    BytesRemainingToBeTransfered = BufferSize;
    while (BytesRemainingToBeTransfered > 0) {

        // Check if the Card is in Ready status
        CmdArg = MmcHostInstance->CardInfo.RCA << 16;
        Response[0] = 0;
        Timeout = 20;
        while ((!(Response[0] & MMC_R0_READY_FOR_DATA))
               && (MMC_R0_CURRENTSTATE(Response) != MMC_R0_STATE_TRAN)
               && Timeout--) {
            Status = MmcHost->SendCommand(MmcHost, MMC_CMD13, CmdArg);
            if (!EFI_ERROR(Status)) {
                MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_R1, Response);
            }
        }

        if (0 == Timeout) {
            DEBUG((EFI_D_ERROR, "MmcDxe: MmcIoBlocks(): The Card is busy\n"));
            return EFI_NOT_READY;
        }

        //Set command argument based on the card access mode (Byte mode or Block mode)
        if (MmcHostInstance->CardInfo.OCRData.AccessMode & BIT1) {
            CmdArg = Lba;
        } else {
            CmdArg = Lba * This->Media->BlockSize;
        }

        if (Transfer == MMC_IOBLOCKS_READ) {
            BlockCount = BytesRemainingToBeTransfered / This->Media->BlockSize;

            // The card is unhappy if trying to read too many blocks at once
            if (BlockCount > MULTI_BLK_XFER_MAX_BLK_CNT) {
                BlockCount = MULTI_BLK_XFER_MAX_BLK_CNT;
            }
            if (BlockCount == 1) {
                // Read a single block
                Cmd = MMC_CMD17;
            } else {
                // Read multiple blocks
                Cmd = MMC_CMD18;
            }
        } else {
            // Write a single block
            Cmd = MMC_CMD24;
        }
        Status = MmcHost->SendCommand(MmcHost, Cmd, CmdArg);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "MmcDxe: MmcIoBlocks(MMC_CMD%d): Error %r\n", Cmd, Status));
            return Status;
        }

        if (Transfer == MMC_IOBLOCKS_READ) {
            Status = MmcNotifyState(MmcHostInstance, MmcSendingDataState);
            if (EFI_ERROR(Status)) {
                DEBUG((EFI_D_ERROR, "MmcDxe: MmcIoBlocks(): Error MmcSendingDataState\n"));
                return Status;
            }

            if (BlockCount == 1) {
                // Read one block of Data
                Status = MmcHost->ReadBlockData(MmcHost, Lba, This->Media->BlockSize, Buffer);
            } else {
                // Read multiple blocks of Data
                for (CurrentBlockNum = 0; CurrentBlockNum < BlockCount; ++CurrentBlockNum) {
                    Status = MmcHost->ReadBlockData(MmcHost, Lba, This->Media->BlockSize, Buffer);
                    if (EFI_ERROR(Status)) {
                        DEBUG((
                            EFI_D_ERROR,
                            "MmcDxe: MmcIoBlocks(): Error Read Multiple Block Data and Status = %r\n",
                            Status));
                        MmcStopTransmission(MmcHost);
                        return Status;
                    }
                    BytesRemainingToBeTransfered -= This->Media->BlockSize;
                    Lba += 1;
                    Buffer = (UINT8 *)Buffer + This->Media->BlockSize;
                }
                MmcStopTransmission(MmcHost);
            }

            if (EFI_ERROR(Status)) {
                DEBUG((EFI_D_ERROR, "MmcDxe: MmcIoBlocks(): Error Read Block Data and Status = %r\n", Status));
                MmcStopTransmission(MmcHost);
                return Status;
            }

        } else {
            Status = MmcNotifyState(MmcHostInstance, MmcReceiveDataState);
            if (EFI_ERROR(Status)) {
                DEBUG((EFI_D_ERROR, "MmcDxe: MmcIoBlocks(): Error MmcProgrammingState\n"));
                return Status;
            }

            // Write one block of Data
            Status = MmcHost->WriteBlockData(MmcHost, Lba, This->Media->BlockSize, Buffer);
            if (EFI_ERROR(Status)) {
                DEBUG((EFI_D_ERROR, "MmcDxe: MmcIoBlocks(): Error Write Block Data and Status = %r\n", Status));
                MmcStopTransmission(MmcHost);
                return Status;
            }
        }

        // Command 13 - Read status and wait for programming to complete (return to tran)
        Timeout = MMCI0_TIMEOUT;
        CmdArg = MmcHostInstance->CardInfo.RCA << 16;
        Response[0] = 0;
        while ((!(Response[0] & MMC_R0_READY_FOR_DATA))
               && (MMC_R0_CURRENTSTATE(Response) != MMC_R0_STATE_TRAN)
               && Timeout--) {
            Status = MmcHost->SendCommand(MmcHost, MMC_CMD13, CmdArg);
            if (!EFI_ERROR(Status)) {
                MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_R1, Response);
                if ((Response[0] & MMC_R0_READY_FOR_DATA)) {
                    break;  // Prevents delay once finished
                }
            }
            NanoSecondDelay(100);
        }

        Status = MmcNotifyState(MmcHostInstance, MmcTransferState);
        if (EFI_ERROR(Status)) {
            DEBUG((EFI_D_ERROR, "MmcDxe: MmcIoBlocks(): Error MmcTransferState\n"));
            return Status;
        }

        // Bookkeeping for multiple block transfer was already performed above
        if (BlockCount == 1) {
            BytesRemainingToBeTransfered -= This->Media->BlockSize;
            Lba += BlockCount;
            Buffer = (UINT8 *)Buffer + This->Media->BlockSize;
        }
    }
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MmcReadBlocks(
    IN EFI_BLOCK_IO_PROTOCOL    *This,
    IN UINT32                   MediaId,
    IN EFI_LBA                  Lba,
    IN UINTN                    BufferSize,
    OUT VOID                    *Buffer
    )
{
#if MMC_BENCHMARK_IO
    static BOOLEAN BenchmarkDone = FALSE;
    if (!BenchmarkDone) {

        DEBUG((EFI_D_INIT, "MmcDxe: Benchmarking BlockIo Read\n"));

        UINT32 CurrByteSize = 512;
        const UINT32 MaxByteSize = 8388608; // 8MB Max
        for (; CurrByteSize <= MaxByteSize; CurrByteSize *= 2) {
            MmcBenchmarkBlockIo(This, MMC_IOBLOCKS_READ, MediaId, CurrByteSize, 10);
        }

        BenchmarkDone = TRUE;
    }
#endif // MMC_BENCHMARK_IO

#if MMC_COLLECT_STATISTICS
    UINT32 NumBlocks = BufferSize / This->Media->BlockSize;
    MMC_HOST_INSTANCE *MmcHostInstance = MMC_HOST_INSTANCE_FROM_BLOCK_IO_THIS(This);
    const UINT32 TableSize = ARRAYSIZE(MmcHostInstance->IoReadStats);
    IoReadStatsEntry *CurrentReadEntry = NULL;

    UINT32 BlockIdx;
    for (BlockIdx = 0; BlockIdx < TableSize; ++BlockIdx) {
        IoReadStatsEntry *Entry = MmcHostInstance->IoReadStats + BlockIdx;
        // Reached end of table and didn't find a match, append an entry
        if (Entry->NumBlocks == 0) {
            ++MmcHostInstance->IoReadStatsNumEntries;
            Entry->NumBlocks = NumBlocks;
        }

        if (Entry->NumBlocks == NumBlocks) {
            CurrentReadEntry = Entry;
            ++Entry->Count;
            break;
        }
    }
    ASSERT(BlockIdx < TableSize);

    UINT64 StartTime = GetPerformanceCounter();

    EFI_STATUS Status = MmcIoBlocks(
        This,
        MMC_IOBLOCKS_READ,
        MediaId,
        Lba,
        BufferSize,
        Buffer
        );
    if (EFI_ERROR(Status)) {
        goto Exit;
    }

    UINT64 EndTime = GetPerformanceCounter();

    ASSERT(CurrentReadEntry != NULL);
    CurrentReadEntry->TotalTransferTimeUs +=
        (UINT32)(((EndTime - StartTime) * 1000000UL) / mHpcTicksPerSeconds);

    //
    // Run statistics and dump updates
    //
    SortIoReadStatsByTotalTransferTime(
        MmcHostInstance->IoReadStats,
        MmcHostInstance->IoReadStatsNumEntries);

    IoReadStatsEntry *MaxNumBlocksEntry = MmcHostInstance->IoReadStats;
    IoReadStatsEntry *MaxCountEntry = MmcHostInstance->IoReadStats;
    UINT32 TotalReadTimeUs = 0;
    UINT32 TotalReadBlocksCount = 0;

    DEBUG((EFI_D_INIT,
           " #Blks\tCnt\tAvg(us)\tAll(us)\n"));

    for (BlockIdx = 0; BlockIdx < MmcHostInstance->IoReadStatsNumEntries; ++BlockIdx) {
        IoReadStatsEntry *CurrEntry = MmcHostInstance->IoReadStats + BlockIdx;

        if (CurrEntry->NumBlocks > MaxNumBlocksEntry->NumBlocks) {
            MaxNumBlocksEntry = CurrEntry;
        }

        if (CurrEntry->Count > MaxCountEntry->Count) {
            MaxCountEntry = CurrEntry;
        }

        TotalReadTimeUs += CurrEntry->TotalTransferTimeUs;
        TotalReadBlocksCount += (CurrEntry->NumBlocks * CurrEntry->Count);

        // Show only the top 5 time consuming transfers
        if (BlockIdx < 5) {
            DEBUG((EFI_D_INIT,
                   " %d\t%d\t%d\t%d\n",
                   (UINT32)CurrEntry->NumBlocks,
                   (UINT32)CurrEntry->Count,
                   (UINT32)(CurrEntry->TotalTransferTimeUs / CurrEntry->Count),
                   (UINT32)CurrEntry->TotalTransferTimeUs));
        }
    }

    DEBUG((EFI_D_INIT,
           "MaxNumBlocksEntry: %d %d %dus\n",
           (UINT32)MaxNumBlocksEntry->NumBlocks,
           (UINT32)MaxNumBlocksEntry->Count,
           (UINT32)MaxNumBlocksEntry->TotalTransferTimeUs));

    DEBUG((EFI_D_INIT,
           "MaxCountEntry: %d %d %dus\n",
           (UINT32)MaxCountEntry->NumBlocks,
           (UINT32)MaxCountEntry->Count,
           (UINT32)MaxCountEntry->TotalTransferTimeUs));

    DEBUG((EFI_D_INIT,
           "UEFI spent %dus~%ds reading %dMB from SDCard\n\n",
           TotalReadTimeUs,
           INT_DIV_ROUND(TotalReadTimeUs, 1000000),
           INT_DIV_ROUND(TotalReadBlocksCount * This->Media->BlockSize, (1024 * 1024))));
Exit:
    return Status;

#else
    return MmcIoBlocks(This, MMC_IOBLOCKS_READ, MediaId, Lba, BufferSize, Buffer);
#endif // MMC_COLLECT_STATISTICS
}

EFI_STATUS
EFIAPI
MmcWriteBlocks(
    IN EFI_BLOCK_IO_PROTOCOL    *This,
    IN UINT32                   MediaId,
    IN EFI_LBA                  Lba,
    IN UINTN                    BufferSize,
    IN VOID                     *Buffer
    )
{
    return MmcIoBlocks(This, MMC_IOBLOCKS_WRITE, MediaId, Lba, BufferSize, Buffer);
}

EFI_STATUS
EFIAPI
MmcFlushBlocks(
    IN EFI_BLOCK_IO_PROTOCOL  *This
    )
{
    return EFI_SUCCESS;
}

VOID
MmcBenchmarkBlockIo(
    IN EFI_BLOCK_IO_PROTOCOL  *This,
    IN UINTN                  Transfer,
    IN UINT32                 MediaId,
    IN UINT32                 BufferByteSize,
    IN UINT32                 Iterations
    )
{
    ASSERT(Iterations > 0);

    EFI_STATUS Status;
    //UINT32 BufferSizeKB = INT_DIV_ROUND(BufferByteSize, 1024);
    VOID* Buffer = AllocateZeroPool(BufferByteSize);
    if (Buffer == NULL) {
        DEBUG((EFI_D_ERROR, "MmcBenchmarkBlockIo() : No enough memory to allocate %dKB buffer\n", BufferSizeKB));
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    UINT32 CurrIteration = Iterations;
    UINT64 TotalTransfersTimeUs = 0;

    while (CurrIteration--) {
        UINT64 StartTime = GetPerformanceCounter();

        Status = MmcIoBlocks(
            This,
            Transfer,
            MediaId,
            0, // Lba
            BufferByteSize,
            Buffer
            );
        if (EFI_ERROR(Status)) {
            goto Exit;
        }

        UINT64 EndTime = GetPerformanceCounter();
        TotalTransfersTimeUs += (((EndTime - StartTime) * 1000000UL) / mHpcTicksPerSeconds);
    }

    /*UINT32 KBps = (UINT32)(((UINT64)BufferSizeKB * (UINT64)Iterations * 1000000UL) / TotalTransfersTimeUs);
    DEBUG((
        EFI_D_INIT,
        "- MmcBenchmarkBlockIo(%a, %dKB)\t: Xfr Avg:%dus\t%dKBps\t%dMBps\n",
        (Transfer == MMC_IOBLOCKS_READ) ? "Read" : "Write",
        BufferSizeKB,
        (UINT32)(TotalTransfersTimeUs / Iterations),
        KBps,
        INT_DIV_ROUND(KBps, 1024)));*/

Exit:
    if (Buffer != NULL) {
        FreePool(Buffer);
    }
}

EFI_STATUS
SdSwitchHighSpeedMode(
    IN  MMC_HOST_INSTANCE   *MmcHostInstance
    )
{
    UINT32                  Response[4];
    UINT8                   SwitchStatus[MMCI0_BLOCKLEN];
    EFI_STATUS              Status;
    UINTN                   CmdArg;
    INTN                    Timeout;
    EFI_MMC_HOST_PROTOCOL   *MmcHost;

    MmcHost = MmcHostInstance->MmcHost;

    // Switching to HighSpeed is valid only in Transfer state
    // Poll until the SDCard is Ready in Transfer state
    CmdArg = MmcHostInstance->CardInfo.RCA << 16;
    Response[0] = 0;
    Timeout = 20;
    while ((!(Response[0] & MMC_R0_READY_FOR_DATA))
           && (MMC_R0_CURRENTSTATE(Response) != MMC_R0_STATE_TRAN)
           && Timeout--) {
        Status = MmcHost->SendCommand(MmcHost, MMC_CMD13, CmdArg);
        if (!EFI_ERROR(Status)) {
            MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_R1, Response);
        }
    }

    if (0 == Timeout) {
        DEBUG((EFI_D_ERROR, "MmcDxe: SdSwitchHighSpeedMode(): Error, the SDCard is busy\n"));
        return EFI_NOT_READY;
    }

    // Set access mode function to HighSpeed and leave other functions as is
    CmdArg =
        SD_CMD6_SET_FUNCTION |
        SD_CMD6_GRP6_NO_INFLUENCE |
        SD_CMD6_GRP5_NO_INFLUENCE |
        SD_CMD6_GRP4_NO_INFLUENCE |
        SD_CMD6_GRP3_NO_INFLUENCE |
        SD_CMD6_GRP2_NO_INFLUENCE |
        SD_CMD6_GRP1_HIGH_SPEED;

    Status = MmcHost->SendCommand(MmcHost, MMC_CMD6, CmdArg);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: SdSwitchHighSpeedMode(): Error, Status = %r\n", Status));
        return Status;
    }

    Status = MmcNotifyState(MmcHostInstance, MmcSendingDataState);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: SdSwitchHighSpeedMode() : Error MmcSendingDataState\n"));
        return Status;
    }

    MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_R1, Response);
    if (Response[0] & MMC_R0_SWITCH_ERROR) {
        DEBUG((
            EFI_D_ERROR,
            "MmcDxe: SdSwitchHighSpeedMode(): MMC_CMD6 response showing Switch Function error\n"));
        return EFI_DEVICE_ERROR;
    }

    // Read back the SwitchState 64-byte (512-bit) data sent by the SDCard on the data line
    // But since SD block size > 64, we will need to read a complete 512-byte block and extract
    // the first 64-byte of it as the SwitchStatus, and ignore the rest
    Status = MmcHost->ReadBlockData(
        MmcHost,
        0,
        MmcHostInstance->BlockIo.Media->BlockSize,
        (UINT32*)SwitchStatus);
    if (EFI_ERROR(Status)) {
        MmcStopTransmission(MmcHost);
        return Status;
    }

    // Read status and wait for programming to complete (return to tran)
    Timeout = MMCI0_TIMEOUT;
    CmdArg = MmcHostInstance->CardInfo.RCA << 16;
    Response[0] = 0;
    while ((!(Response[0] & MMC_R0_READY_FOR_DATA))
           && (MMC_R0_CURRENTSTATE(Response) != MMC_R0_STATE_TRAN)
           && Timeout--) {
        Status = MmcHost->SendCommand(MmcHost, MMC_CMD13, CmdArg);
        if (!EFI_ERROR(Status)) {
            MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_R1, Response);
        }
    }

    if (0 == Timeout) {
        DEBUG((EFI_D_ERROR, "MmcDxe: SdSwitchHighSpeedMode(): Error, the SDCard is busy\n"));
        return EFI_NOT_READY;
    }

    Status = MmcNotifyState(MmcHostInstance, MmcTransferState);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: SdSwitchHighSpeedMode(): Error MmcTransferState\n"));
        return Status;
    }

    // Per SD specs, should wait at least 8 clocks for Switch Function to complete after
    // the end-bit of status data
    // Assuming SDCard still running on lower speed before the switch of 25MHz to 50MHz
    // 8 clocks = 8 * 1/25000000 * 1000000000 ~= 320ns
    NanoSecondDelay(400);

    // Special RCA to deselect the card and move it back to StandBy state
    CmdArg = 0x0000 << 16;
    Status = MmcHost->SendCommand(MmcHost, MMC_CMD7, CmdArg);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: SdSwitchHighSpeedMode() : Error, Status=%r\n", Status));
        return Status;
    }

    Status = MmcNotifyState(MmcHostInstance, MmcStandByState);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: SdSwitchHighSpeedMode() : Error MmcStandByState\n"));
        return Status;
    }

    // Send a command to get Card specific data with the updated TRAN_SPEED after
    // switching to HighSpeed mode
    CmdArg = MmcHostInstance->CardInfo.RCA << 16;
    Status = MmcHost->SendCommand(MmcHost, MMC_CMD9, CmdArg);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: SdSwitchHighSpeedMode(): Error, Status=%r\n", Status));
        return Status;
    }

    MmcHost->ReceiveResponse(MmcHost, MMC_RESPONSE_TYPE_CSD, Response);
    PrintCSD((CSD*)Response);

    // Switch back to Transfer state with the new HighSpeed mode
    CmdArg = MmcHostInstance->CardInfo.RCA << 16;
    Status = MmcHost->SendCommand(MmcHost, MMC_CMD7, CmdArg);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: SdSwitchHighSpeedMode(): Error, Status = %r\n", Status));
        return Status;
    }

    Status = MmcNotifyState(MmcHostInstance, MmcTransferState);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "MmcDxe: SdSwitchHighSpeedMode(): Error MmcTransferState\n"));
        return Status;
    }

    return EFI_SUCCESS;
}
