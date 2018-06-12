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

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DmaLib.h>
#include <Library/TimerLib.h>

#include <Protocol/EmbeddedExternalDevice.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/MmcHost.h>
#include <Protocol/RaspberryPiFirmware.h>

#include <IndustryStandard/Bcm2836.h>
#include <IndustryStandard/RpiFirmware.h>
#include <IndustryStandard/Bcm2836SdHost.h>
#include <IndustryStandard/Bcm2836Gpio.h>

#define SDHOST_BLOCK_BYTE_LENGTH            512

// Driver Timing Parameters
#define CMD_STALL_AFTER_POLL_US             1
#define CMD_MIN_POLL_TOTAL_TIME_US          100000 // 100ms
#define CMD_MAX_POLL_COUNT                  (CMD_MIN_POLL_TOTAL_TIME_US / CMD_STALL_AFTER_POLL_US)
#define CMD_MAX_RETRY_COUNT                 3
#define CMD_STALL_AFTER_RETRY_US            20 // 20us
#define FIFO_MAX_POLL_COUNT                 1000000
#define STALL_TO_STABILIZE_US               10000 // 10ms

#define IDENT_MODE_SD_CLOCK_FREQ_HZ         400000 // 400KHz

// Macros adopted from MmcDxe internal header
#define SDHOST_CSD_GET_TRANSPEED(Response)  ((Response[2] >> 24)& 0xFF)
#define SDHOST_R0_READY_FOR_DATA            BIT8
#define SDHOST_R0_CURRENTSTATE(Response)    ((Response >> 9) & 0xF)
#define SDHOST_RCA_SHIFT                    16

#ifndef MMC_ACMD6
#define MMC_ACMD6 (MMC_INDX(6) | MMC_CMD_WAIT_RESPONSE | MMC_CMD_NO_CRC_RESPONSE)
#endif

#define DEBUG_MMCHOST_SD DEBUG_ERROR

STATIC RASPBERRY_PI_FIRMWARE_PROTOCOL   *mFwProtocol;

// Per Physical Layer Simplified Specs
CONST CHAR8* mStrSdState[] = { "idle", "ready", "ident", "stby", "tran", "data", "rcv", "prg", "dis", "ina" };
UINT8 mMaxDataTransferRate = 0;
UINT32 mReconfigSdClock = FALSE;
UINT32 mRca = 0;
UINT32 mLastExecutedMmcCmd = MMC_GET_INDX(MMC_CMD0);
BOOLEAN mIsSdBusSwitched4BitMode = FALSE;
UINT64 mScr = 0;

EFI_STATUS SdHostGetSdStatus(UINT32* StatusR0);

BOOLEAN IsAppCmd() { return mLastExecutedMmcCmd == MMC_CMD55; }

BOOLEAN IsBusyCmd(UINT32 MmcCmd) { return ((MmcCmd == MMC_CMD7 || MmcCmd == MMC_CMD12) && !IsAppCmd()); }

BOOLEAN IsWriteCmd(UINT32 MmcCmd) { return (MmcCmd == MMC_CMD24 && !IsAppCmd()); }

BOOLEAN IsReadCmd(UINT32 MmcCmd)
{
    BOOLEAN CmdIsAppCmd = IsAppCmd();
    return
        (MmcCmd == MMC_CMD6 && !CmdIsAppCmd) ||
        (MmcCmd == MMC_CMD17 && !CmdIsAppCmd) ||
        (MmcCmd == MMC_CMD18 && !CmdIsAppCmd) ||
        (MmcCmd == MMC_CMD13 && CmdIsAppCmd);
}

VOID
SdHostDumpRegisters(
    VOID
    )
{
    DEBUG((DEBUG_MMCHOST_SD, "SdHost: Registers Dump:\n"));
    DEBUG((DEBUG_MMCHOST_SD, "  CMD:  0x%8.8X\n", MmioRead32(SDHOST_CMD)));
    DEBUG((DEBUG_MMCHOST_SD, "  ARG:  0x%8.8X\n", MmioRead32(SDHOST_ARG)));
    DEBUG((DEBUG_MMCHOST_SD, "  TOUT: 0x%8.8X\n", MmioRead32(SDHOST_TOUT)));
    DEBUG((DEBUG_MMCHOST_SD, "  CDIV: 0x%8.8X\n", MmioRead32(SDHOST_CDIV)));
    DEBUG((DEBUG_MMCHOST_SD, "  RSP0: 0x%8.8X\n", MmioRead32(SDHOST_RSP0)));
    DEBUG((DEBUG_MMCHOST_SD, "  RSP1: 0x%8.8X\n", MmioRead32(SDHOST_RSP1)));
    DEBUG((DEBUG_MMCHOST_SD, "  RSP2: 0x%8.8X\n", MmioRead32(SDHOST_RSP2)));
    DEBUG((DEBUG_MMCHOST_SD, "  RSP3: 0x%8.8X\n", MmioRead32(SDHOST_RSP3)));
    DEBUG((DEBUG_MMCHOST_SD, "  HSTS: 0x%8.8X\n", MmioRead32(SDHOST_HSTS)));
    DEBUG((DEBUG_MMCHOST_SD, "  VDD:  0x%8.8X\n", MmioRead32(SDHOST_VDD)));
    DEBUG((DEBUG_MMCHOST_SD, "  EDM:  0x%8.8X\n", MmioRead32(SDHOST_EDM)));
    DEBUG((DEBUG_MMCHOST_SD, "  HCFG: 0x%8.8X\n", MmioRead32(SDHOST_HCFG)));
    DEBUG((DEBUG_MMCHOST_SD, "  HBCT: 0x%8.8X\n", MmioRead32(SDHOST_HBCT)));
    DEBUG((DEBUG_MMCHOST_SD, "  HBLC: 0x%8.8X\n\n", MmioRead32(SDHOST_HBLC)));
}

VOID
SdHostDumpSdCardStatus(
    VOID
    )
{
    UINT32 SdCardStatus;
    EFI_STATUS Status = SdHostGetSdStatus(&SdCardStatus);
    if (!EFI_ERROR(Status)) {
        /*UINT32 CurrState = SDHOST_R0_CURRENTSTATE(SdCardStatus);
        DEBUG((
            DEBUG_MMCHOST_SD,
            "SdHost: SdCardStatus 0x%8.8X: ReadyForData?%d, State[%d]: %a\n",
            SdCardStatus,
            ((SdCardStatus & SDHOST_R0_READY_FOR_DATA) ? 1 : 0),
            CurrState,
            ((CurrState < (sizeof(mStrSdState) / sizeof(*mStrSdState))) ?
                mStrSdState[CurrState] : "UNDEF")));*/
    }
}

VOID
SdHostDumpStatus(
    VOID
    )
{
    SdHostDumpRegisters();

    UINT32 Hsts = MmioRead32(SDHOST_HSTS);

    if (Hsts & SDHOST_HSTS_ERROR) {
        DEBUG((DEBUG_MMCHOST_SD, "SdHost: Diagnose HSTS: 0x%8.8X\n", Hsts));

        if (Hsts & SDHOST_HSTS_FIFO_ERROR)
            DEBUG((DEBUG_MMCHOST_SD, "  - Fifo Error\n"));
        if (Hsts & SDHOST_HSTS_CRC7_ERROR)
            DEBUG((DEBUG_MMCHOST_SD, "  - CRC7 Error\n"));
        if (Hsts & SDHOST_HSTS_CRC16_ERROR)
            DEBUG((DEBUG_MMCHOST_SD, "  - CRC16 Error\n"));
        if (Hsts & SDHOST_HSTS_CMD_TIME_OUT)
            DEBUG((DEBUG_MMCHOST_SD, "  - CMD Timeout\n"));
        if (Hsts & SDHOST_HSTS_REW_TIME_OUT)
            DEBUG((DEBUG_MMCHOST_SD, "  - Read/Erase/Write Transfer Timeout\n"));
    }

    /*UINT32 Edm = MmioRead32(SDHOST_EDM);
    DEBUG((DEBUG_MMCHOST_SD, "SdHost: Diagnose EDM: 0x%8.8X\n", Edm));
    DEBUG((DEBUG_MMCHOST_SD, "  - FSM: 0x%x\n", (Edm & 0xF)));
    DEBUG((DEBUG_MMCHOST_SD, "  - Fifo Count: %d\n", ((Edm >> 4) & 0x1F)));
    DEBUG((DEBUG_MMCHOST_SD,
        "  - Fifo Write Threshold: %d\n",
        ((Edm >> SDHOST_EDM_WRITE_THRESHOLD_SHIFT) & SDHOST_EDM_THRESHOLD_MASK)));
    DEBUG((DEBUG_MMCHOST_SD,
        "  - Fifo Read Threshold: %d\n",
        ((Edm >> SDHOST_EDM_READ_THRESHOLD_SHIFT) & SDHOST_EDM_THRESHOLD_MASK)));*/

    SdHostDumpSdCardStatus();
}

EFI_STATUS
SdHostSetClockFrequency(
    IN UINTN TargetSdFreqHz
    )
{
    EFI_STATUS Status;
    UINT32 CoreClockFreqHz = 0;

    // First figure out the core clock
    Status = mFwProtocol->GetClockRate(RPI_FW_CLOCK_RATE_CORE, &CoreClockFreqHz);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    ASSERT (CoreClockFreqHz != 0);

    // fSDCLK = fcore_pclk/(ClockDiv+2)
    UINT32 ClockDiv = (CoreClockFreqHz - (2 * TargetSdFreqHz)) / TargetSdFreqHz;
    UINT32 ActualSdFreqHz = CoreClockFreqHz / (ClockDiv + 2);

    DEBUG((
        DEBUG_MMCHOST_SD,
        "SdHost: CoreClock=%dHz, CDIV=%d, Requested SdClock=%dHz, Actual SdClock=%dHz\n",
        CoreClockFreqHz,
        ClockDiv,
        TargetSdFreqHz,
        ActualSdFreqHz));

    MmioWrite32(SDHOST_CDIV, ClockDiv);
    // Set timeout after 1 second, i.e ActualSdFreqHz SD clock cycles
    MmioWrite32(SDHOST_TOUT, ActualSdFreqHz);

    return Status;
}

BOOLEAN
SdIsCardPresent(
    IN EFI_MMC_HOST_PROTOCOL *This
    )
{
    BOOLEAN IsCardPresent = TRUE;
    DEBUG((DEBUG_MMCHOST_SD, "SdHost: SdIsCardPresent(): %d\n", IsCardPresent));
    return IsCardPresent;
}

BOOLEAN
SdIsReadOnly(
    IN EFI_MMC_HOST_PROTOCOL *This
    )
{
    BOOLEAN IsReadOnly = FALSE;
    DEBUG((DEBUG_MMCHOST_SD, "SdHost: SdIsReadOnly(): %d\n", IsReadOnly));
    return IsReadOnly;
}

EFI_STATUS
SdBuildDevicePath(
    IN EFI_MMC_HOST_PROTOCOL       *This,
    IN EFI_DEVICE_PATH_PROTOCOL    **DevicePath
    )
{
    EFI_DEVICE_PATH_PROTOCOL *NewDevicePathNode;
    EFI_GUID DevicePathGuid = EFI_CALLER_ID_GUID;

    DEBUG((DEBUG_MMCHOST_SD, "SdHost: SdBuildDevicePath()\n"));

    NewDevicePathNode = CreateDeviceNode(HARDWARE_DEVICE_PATH, HW_VENDOR_DP, sizeof(VENDOR_DEVICE_PATH));
    CopyGuid(&((VENDOR_DEVICE_PATH*)NewDevicePathNode)->Guid, &DevicePathGuid);
    *DevicePath = NewDevicePathNode;

    return EFI_SUCCESS;
}

EFI_STATUS
SdSendCommand(
    IN EFI_MMC_HOST_PROTOCOL    *This,
    IN MMC_CMD                  MmcCmd,
    IN UINT32                   Argument
    )
{
    // Fail fast, CMD5 (CMD_IO_SEND_OP_COND) is only valid for SDIO cards and thus expected to always fail
    if (MmcCmd == MMC_CMD5)
    {
        DEBUG((
            DEBUG_MMCHOST_SD,
            "SdHost: SdSendCommand(CMD%d, Argument: %08x) ignored\n",
            MMC_GET_INDX(MmcCmd),
            Argument));
        return EFI_UNSUPPORTED;
    }

    if (MmioRead32(SDHOST_CMD) & SDHOST_CMD_NEW_FLAG) {
        DEBUG((
            DEBUG_ERROR,
            "SdHost: SdSendCommand(): Failed to execute CMD%d, a CMD is already being executed.\n",
            MMC_GET_INDX(MmcCmd)));
        SdHostDumpStatus();
        return EFI_DEVICE_ERROR;
    }

    // Write command argument
    MmioWrite32(SDHOST_ARG, Argument);

    UINT32 SdCmd = 0;
    {
        // Set response type
        if (MmcCmd & MMC_CMD_WAIT_RESPONSE) {
            if (MmcCmd & MMC_CMD_LONG_RESPONSE) {
                SdCmd |= SDHOST_CMD_RESPONSE_CMD_LONG_RESP;
            }
        } else {
            SdCmd |= SDHOST_CMD_RESPONSE_CMD_NO_RESP;
        }

        if (IsBusyCmd(MmcCmd))
            SdCmd |= SDHOST_CMD_BUSY_CMD;

        if (IsReadCmd(MmcCmd))
            SdCmd |= SDHOST_CMD_READ_CMD;

        if (IsWriteCmd(MmcCmd))
            SdCmd |= SDHOST_CMD_WRITE_CMD;

        SdCmd |= MMC_GET_INDX(MmcCmd);
    }

    DEBUG((
        DEBUG_MMCHOST_SD,
        "SdHost: SdSendCommand(CMD%d, Argument: %08x): BUSY=%d, RESP=%d, WRITE=%d, READ=%d\n",
        MMC_GET_INDX(MmcCmd),
        Argument,
        ((SdCmd & SDHOST_CMD_BUSY_CMD) ? 1 : 0),
        ((SdCmd & (SDHOST_CMD_RESPONSE_CMD_LONG_RESP | SDHOST_CMD_RESPONSE_CMD_NO_RESP)) >> 9),
        ((SdCmd & SDHOST_CMD_WRITE_CMD) ? 1 : 0),
        ((SdCmd & SDHOST_CMD_READ_CMD) ? 1 : 0)));

    UINT32 PollCount = 0;
    UINT32 RetryCount = 0;
    BOOLEAN IsCmdExecuted = FALSE;
    EFI_STATUS Status = EFI_SUCCESS;

    // Keep retrying the command untill it succeed
    while ((RetryCount < CMD_MAX_RETRY_COUNT) && !IsCmdExecuted) {
        // Clear prev cmd status
        MmioWrite32(SDHOST_HSTS, SDHOST_HSTS_CLEAR);

        if (IsReadCmd(MmcCmd) || IsWriteCmd(MmcCmd)) {
            // Flush Fifo if this cmd will start a new transfer in case
            // there is stale bytes in the Fifo
            MmioOr32(SDHOST_EDM, SDHOST_EDM_FIFO_CLEAR);
        }

        // Write command and set it to start execution
        MmioWrite32(SDHOST_CMD, SDHOST_CMD_NEW_FLAG | SdCmd);

        // Poll for the command status untill it finishes execution
        while (PollCount < CMD_MAX_POLL_COUNT) {
            UINT32 CmdReg = MmioRead32(SDHOST_CMD);

            // Read status of command response
            if (CmdReg & SDHOST_CMD_FAIL_FLAG) {
                Status = EFI_DEVICE_ERROR;
                break;
            }

            // Check if command is completed.
            if (!(CmdReg & SDHOST_CMD_NEW_FLAG)) {
                IsCmdExecuted = TRUE;
                break;
            }

            ++PollCount;
            gBS->Stall(CMD_STALL_AFTER_POLL_US);
        }

        if (!IsCmdExecuted) {
            ++RetryCount;
            gBS->Stall(CMD_STALL_AFTER_RETRY_US);
        }
    }

    if (RetryCount == CMD_MAX_RETRY_COUNT) {
        Status = EFI_TIMEOUT;
    }



    if (EFI_ERROR(Status) ||
        (MmioRead32(SDHOST_HSTS) & SDHOST_HSTS_ERROR)) {
        // Deselecting the SDCard with CMD7 and RCA=0x0 always timeout on SDHost
        if (MmcCmd == MMC_CMD7 &&
            Argument == 0) {
            Status = EFI_SUCCESS;

        } else {
            DEBUG((
                DEBUG_ERROR,
                "SdHost: SdSendCommand(): CMD%d execution failed after %d trial(s)\n",
                MMC_GET_INDX(MmcCmd),
                RetryCount));
            SdHostDumpStatus();
        }

        MmioWrite32(SDHOST_HSTS, SDHOST_HSTS_CLEAR);

    } else if (IsCmdExecuted) {
        ASSERT(!(MmioRead32(SDHOST_HSTS) & SDHOST_HSTS_ERROR));
        mLastExecutedMmcCmd = MmcCmd;

    } else {
        ASSERT("SdHost: SdSendCommand(): Unexpected State");
        SdHostDumpStatus();
    }

    return Status;
}

EFI_STATUS
SdReceiveResponse(
    IN EFI_MMC_HOST_PROTOCOL    *This,
    IN MMC_RESPONSE_TYPE        Type,
    IN UINT32*                  Buffer
    )
{
    if (Buffer == NULL) {
        DEBUG((DEBUG_ERROR, "SdHost: SdReceiveResponse(): Input Buffer is NULL\n"));
        return EFI_INVALID_PARAMETER;
    }

    if ((Type == MMC_RESPONSE_TYPE_R1) ||
        (Type == MMC_RESPONSE_TYPE_R1b) ||
        (Type == MMC_RESPONSE_TYPE_R3) ||
        (Type == MMC_RESPONSE_TYPE_R6) ||
        (Type == MMC_RESPONSE_TYPE_R7)) {
        Buffer[0] = MmioRead32(SDHOST_RSP0);
        DEBUG((
            DEBUG_MMCHOST_SD,
            "SdHost: SdReceiveResponse(Type: %x), Buffer[0]: %08x\n",
            Type, Buffer[0]));

    } else if (Type == MMC_RESPONSE_TYPE_R2) {
        Buffer[0] = MmioRead32(SDHOST_RSP0);
        Buffer[1] = MmioRead32(SDHOST_RSP1);
        Buffer[2] = MmioRead32(SDHOST_RSP2);
        Buffer[3] = MmioRead32(SDHOST_RSP3);

        //
        // Shift the whole response right 8-bits to strip down CRC. It is common for standard
        // SDHCs to not store in the RSP registers the first 8-bits for R2 responses CID[0:7]
        // and CSD[0:7] since those 8-bits contain the CRC which is already handled by the SDHC HW FSM
        //
        UINT8 *BufferAsBytes = (UINT8*)Buffer;
        const UINT32 BufferSizeMax = sizeof(UINT32) * 4;
        UINT32 ByteIdx;
        for (ByteIdx = 0; ByteIdx < BufferSizeMax - 1; ++ByteIdx) {
            BufferAsBytes[ByteIdx] = BufferAsBytes[ByteIdx + 1];
        }
        BufferAsBytes[BufferSizeMax - 1] = 0;

        DEBUG((
            DEBUG_MMCHOST_SD,
            "SdHost: SdReceiveResponse(Type: %x), Buffer[0-3]: %08x, %08x, %08x, %08x\n",
            Type, Buffer[0], Buffer[1], Buffer[2], Buffer[3]));
    }

    // During initialization per Specs, MmcDxe will select the SDCard
    // and ask it to publish an RCA address for further commands
    if (mLastExecutedMmcCmd == MMC_CMD3) {
        mRca = Buffer[0] >> SDHOST_RCA_SHIFT;
    }

    // MmcDxe will be querying for CSD register during initialization,
    // this is a good place to capture the SDCard transfer rate fields
    if (mLastExecutedMmcCmd == MMC_CMD9) {
        UINT32 NewMaxDataTransferRate = SDHOST_CSD_GET_TRANSPEED(Buffer);
        if (NewMaxDataTransferRate != mMaxDataTransferRate) {
            DEBUG((
                DEBUG_MMCHOST_SD,
                "SdHost: SdReceiveResponse(), TRAN_SPEED got updated from %X"
                " to %X and SD clock needs reprogramming\n",
                mMaxDataTransferRate,
                NewMaxDataTransferRate));

            mReconfigSdClock = TRUE;
            mMaxDataTransferRate = NewMaxDataTransferRate;
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS
SdHostGetSdStatus(
    UINT32* SdStatus
    )
{
    ASSERT(mRca != 0);
    ASSERT(SdStatus != NULL);

    // On command completion with R1 or R1b response type
    // the SDCard status will be in RSP0
    UINT32 Rsp0 = MmioRead32(SDHOST_RSP0);
    if (Rsp0 != 0xFFFFFFFF) {
        *SdStatus = Rsp0;
        return EFI_SUCCESS;
    }

    return EFI_NO_RESPONSE;
}

EFI_STATUS
CalculateSdCardMaxFreq(
    UINT32* SdClkFreqHz
    )
{
    UINT32 TransferRateBitPerSecond = 0;
    UINT32 TimeValue = 0;

    ASSERT(SdClkFreqHz != NULL);
    ASSERT(mMaxDataTransferRate != 0);

    // Calculate Transfer rate unit (Bits 2:0 of TRAN_SPEED)
    switch (mMaxDataTransferRate & 0x7) { // 2
    case 0: // 100kbit/s
        TransferRateBitPerSecond = 100 * 1000;
        break;

    case 1: // 1Mbit/s
        TransferRateBitPerSecond = 1 * 1000 * 1000;
        break;

    case 2: // 10Mbit/s
        TransferRateBitPerSecond = 10 * 1000 * 1000;
        break;

    case 3: // 100Mbit/s
        TransferRateBitPerSecond = 100 * 1000 * 1000;
        break;

    default:
        DEBUG((DEBUG_ERROR, "SdHost: CalculateSdCardMaxFreq(): Invalid parameter\n"));
        ASSERT(FALSE);
        return EFI_INVALID_PARAMETER;
    }

    //Calculate Time value (Bits 6:3 of TRAN_SPEED)
    switch ((mMaxDataTransferRate >> 3) & 0xF) { // 6
    case 0x1:
        TimeValue = 10;
        break;

    case 0x2:
        TimeValue = 12;
        break;

    case 0x3:
        TimeValue = 13;
        break;

    case 0x4:
        TimeValue = 15;
        break;

    case 0x5:
        TimeValue = 20;
        break;

    case 0x6:
        TimeValue = 25;
        break;

    case 0x7:
        TimeValue = 30;
        break;

    case 0x8:
        TimeValue = 35;
        break;

    case 0x9:
        TimeValue = 40;
        break;

    case 0xA:
        TimeValue = 45;
        break;

    case 0xB:
        TimeValue = 50;
        break;

    case 0xC:
        TimeValue = 55;
        break;

    case 0xD:
        TimeValue = 60;
        break;

    case 0xE:
        TimeValue = 70;
        break;

    case 0xF:
        TimeValue = 80;
        break;

    default:
        DEBUG((DEBUG_ERROR, "SdHost: CalculateSdCardMaxFreq(): Invalid parameter\n"));
        ASSERT(FALSE);
        return EFI_INVALID_PARAMETER;
    }

    *SdClkFreqHz = (TransferRateBitPerSecond * TimeValue) / 10;

    DEBUG((
        DEBUG_MMCHOST_SD,
        "SdHost: TransferRateUnitId=%d TimeValue*10=%d, SdCardFrequency=%dKHz\n",
        (mMaxDataTransferRate & 0x7),
        TimeValue,
        *SdClkFreqHz / 1000));

    return EFI_SUCCESS;
}

EFI_STATUS
SdHostSwitch4BitBusWidth(
    VOID
    )
{
    ASSERT(mRca != 0);

    // Tell the SDCard to interpret the next command as an application
    // specific command
    UINT32 CmdArg = mRca << SDHOST_RCA_SHIFT;
    EFI_STATUS Status = SdSendCommand(NULL, MMC_CMD55, CmdArg);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    CmdArg = 0x2; // 4-Bit Bus Width Flag
    Status = SdSendCommand(NULL, MMC_ACMD6, CmdArg);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    MmioOr32(SDHOST_HCFG, SDHOST_HCFG_WIDE_EXT_BUS);

    return Status;
}

EFI_STATUS
SdReadBlockData(
    IN EFI_MMC_HOST_PROTOCOL    *This,
    IN EFI_LBA                  Lba,
    IN UINTN                    Length,
    IN UINT32*                  Buffer
    )
{
    DEBUG((
        DEBUG_MMCHOST_SD,
        "SdHost: SdReadBlockData(LBA: 0x%x, Length: 0x%x, Buffer: 0x%x)\n",
        (UINT32)Lba, Length, Buffer));

    ASSERT(Buffer != NULL);
    ASSERT(Length % SDHOST_BLOCK_BYTE_LENGTH == 0);

    EFI_STATUS Status = EFI_SUCCESS;

    mFwProtocol->SetLed(TRUE);
    {
        UINT32 NumWords = Length / 4;
        UINT32 WordIdx;

        for (WordIdx = 0; WordIdx < NumWords; ++WordIdx) {
            UINT32 PollCount = 0;
            while (PollCount < FIFO_MAX_POLL_COUNT) {
                if (MmioRead32(SDHOST_HSTS) & SDHOST_HSTS_DATA_FLAG) {
                    Buffer[WordIdx] = MmioRead32(SDHOST_DATA);
                    break;
                }

                ++PollCount;
            }

            if (PollCount == FIFO_MAX_POLL_COUNT) {
                DEBUG(
                    (DEBUG_ERROR,
                        "SdHost: SdReadBlockData(): Block Word%d read poll timed-out\n",
                        WordIdx));
                SdHostDumpStatus();
                MmioWrite32(SDHOST_HSTS, SDHOST_HSTS_CLEAR);
                Status = EFI_TIMEOUT;
                break;
            }
        }
    }
    mFwProtocol->SetLed(FALSE);

    return Status;
}

EFI_STATUS
SdWriteBlockData(
    IN EFI_MMC_HOST_PROTOCOL    *This,
    IN EFI_LBA                  Lba,
    IN UINTN                    Length,
    IN UINT32*                  Buffer
    )
{
    DEBUG((
        DEBUG_MMCHOST_SD,
        "SdHost: SdWriteBlockData(LBA: 0x%x, Length: 0x%x, Buffer: 0x%x)\n",
        (UINT32)Lba, Length, Buffer));

    ASSERT(Buffer != NULL);
    ASSERT(Length % SDHOST_BLOCK_BYTE_LENGTH == 0);

    EFI_STATUS Status = EFI_SUCCESS;

    //    LedSetOk(TRUE);
    {
        UINT32 NumWords = Length / 4;
        UINT32 WordIdx;

        for (WordIdx = 0; WordIdx < NumWords; ++WordIdx) {
            UINT32 PollCount = 0;
            while (PollCount < FIFO_MAX_POLL_COUNT) {
                if (MmioRead32(SDHOST_HSTS) & SDHOST_HSTS_DATA_FLAG) {
                    MmioWrite32(SDHOST_DATA, Buffer[WordIdx]);
                    break;
                }

                ++PollCount;
            }

            if (PollCount == FIFO_MAX_POLL_COUNT) {
                DEBUG((
                    DEBUG_ERROR,
                    "SdHost: SdWriteBlockData(): Block Word%d write poll timed-out\n",
                    WordIdx));
                SdHostDumpStatus();
                MmioWrite32(SDHOST_HSTS, SDHOST_HSTS_CLEAR);
                Status = EFI_TIMEOUT;
                break;
            }
        }
    }
    mFwProtocol->SetLed(FALSE);

    return Status;
}

EFI_STATUS
SdNotifyState(
    IN EFI_MMC_HOST_PROTOCOL    *This,
    IN MMC_STATE                State
    )
{
    DEBUG((DEBUG_MMCHOST_SD, "SdHost: SdNotifyState(State: %d) ", State));

    switch (State) {
    case MmcHwInitializationState:
    {
        DEBUG((DEBUG_MMCHOST_SD, "MmcHwInitializationState\n", State));

        // Turn-off SD Card power
        MmioWrite32(SDHOST_VDD, 0);
        {
            // Reset command and arg
            MmioWrite32(SDHOST_CMD, 0);
            MmioWrite32(SDHOST_ARG, 0);
            // Reset clock divider
            MmioWrite32(SDHOST_CDIV, 0);
            // Clear status flags
            MmioWrite32(SDHOST_HSTS, SDHOST_HSTS_CLEAR);;
            // Reset controller configs
            MmioWrite32(SDHOST_HCFG, 0);
            MmioWrite32(SDHOST_HBCT, 0);
            MmioWrite32(SDHOST_HBLC, 0);

            gBS->Stall(STALL_TO_STABILIZE_US);
        }
        // Turn-on SD Card power
        MmioWrite32(SDHOST_VDD, 1);

        gBS->Stall(STALL_TO_STABILIZE_US);

        // Write controller configs
        UINT32 Hcfg = 0;
        Hcfg |= SDHOST_HCFG_WIDE_INT_BUS;
        Hcfg |= SDHOST_HCFG_SLOW_CARD; // Use all bits of CDIV in DataMode
        MmioWrite32(SDHOST_HCFG, Hcfg);

        // Set default clock frequency
        EFI_STATUS Status = SdHostSetClockFrequency(IDENT_MODE_SD_CLOCK_FREQ_HZ);
        if (EFI_ERROR(Status)) {
            DEBUG((
                DEBUG_ERROR,
                "SdHost: SdNotifyState(): Fail to initialize SD clock to %dHz\n",
                IDENT_MODE_SD_CLOCK_FREQ_HZ));
            SdHostDumpStatus();
            return Status;
        }
    }
    break;
    case MmcIdleState:
        DEBUG((DEBUG_MMCHOST_SD, "MmcIdleState\n", State));
        break;
    case MmcReadyState:
        DEBUG((DEBUG_MMCHOST_SD, "MmcReadyState\n", State));
        break;
    case MmcIdentificationState:
        DEBUG((DEBUG_MMCHOST_SD, "MmcIdentificationState\n", State));
        break;
    case MmcStandByState:
        DEBUG((DEBUG_MMCHOST_SD, "MmcStandByState\n", State));
        break;
    case MmcTransferState:
    {
        DEBUG((DEBUG_MMCHOST_SD, "MmcTransferState\n", State));

        // Switch to 4-Bit mode if not switched yet to use DAT[0-3] lines
        // Bus width switch is only allowed in transfer state
        if (!mIsSdBusSwitched4BitMode) {
            EFI_STATUS Status = SdHostSwitch4BitBusWidth();
            if (EFI_ERROR(Status)) {
                DEBUG((
                    DEBUG_ERROR,
                    "SdHost: SdNotifyState(): Failed to switch to 4-Bit mode\n"));
                SdHostDumpStatus();
                return Status;
            }

            DEBUG((DEBUG_INIT, "SdHost: Switched SDCard to 4-Bit Mode\n"));

            mIsSdBusSwitched4BitMode = TRUE;
        }

        if (mReconfigSdClock) {

            UINT32 sdCardMaxClockFreqHz;
            EFI_STATUS Status;

            Status = CalculateSdCardMaxFreq(&sdCardMaxClockFreqHz);
            if (EFI_ERROR(Status)) {
                DEBUG((
                    DEBUG_ERROR,
                    "SdHost: SdNotifyState(): Failed to retrieve SD Card max frequency\n"));
                SdHostDumpStatus();
                return Status;
            }

            Status = SdHostSetClockFrequency(sdCardMaxClockFreqHz);
            if (EFI_ERROR(Status)) {
                DEBUG((
                    DEBUG_ERROR,
                    "SdHost: SdNotifyState(): Failed to configure SD clock to %dHz\n",
                    sdCardMaxClockFreqHz));
                SdHostDumpStatus();
                return Status;
            }

            mReconfigSdClock = FALSE;
        }
    }
    break;
    case MmcSendingDataState:
        DEBUG((DEBUG_MMCHOST_SD, "MmcSendingDataState\n", State));
        break;
    case MmcReceiveDataState:
        DEBUG((DEBUG_MMCHOST_SD, "MmcReceiveDataState\n", State));
        break;
    case MmcProgrammingState:
        DEBUG((DEBUG_MMCHOST_SD, "MmcProgrammingState\n", State));
        break;
    case MmcDisconnectState:
    case MmcInvalidState:
    default:
        DEBUG((DEBUG_ERROR, "SdHost: SdNotifyState(): Invalid State: %d\n", State));
        ASSERT(0);
    }

    return EFI_SUCCESS;
}

EFI_MMC_HOST_PROTOCOL gMmcHost =
{
    MMC_HOST_PROTOCOL_REVISION,
    SdIsCardPresent,
    SdIsReadOnly,
    SdBuildDevicePath,
    SdNotifyState,
    SdSendCommand,
    SdReceiveResponse,
    SdReadBlockData,
    SdWriteBlockData
};

EFI_STATUS
SdHostInitialize(
    IN EFI_HANDLE          ImageHandle,
    IN EFI_SYSTEM_TABLE    *SystemTable
    )
{
    EFI_STATUS Status;
    EFI_HANDLE Handle = NULL;

    Status = gBS->LocateProtocol (&gRaspberryPiFirmwareProtocolGuid, NULL,
                                  (VOID **)&mFwProtocol);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG((DEBUG_MMCHOST_SD, "SdHost: Initialize\n"));
    DEBUG((DEBUG_MMCHOST_SD, "Config:\n"));
    DEBUG((DEBUG_MMCHOST_SD, " - FIFO_MAX_POLL_COUNT=%d\n", FIFO_MAX_POLL_COUNT));
    DEBUG((DEBUG_MMCHOST_SD, " - CMD_STALL_AFTER_POLL_US=%dus\n", CMD_STALL_AFTER_POLL_US));
    DEBUG((DEBUG_MMCHOST_SD, " - CMD_MIN_POLL_TOTAL_TIME_US=%dms\n", CMD_MIN_POLL_TOTAL_TIME_US / 1000));
    DEBUG((DEBUG_MMCHOST_SD, " - CMD_MAX_POLL_COUNT=%d\n", CMD_MAX_POLL_COUNT));
    DEBUG((DEBUG_MMCHOST_SD, " - CMD_MAX_RETRY_COUNT=%d\n", CMD_MAX_RETRY_COUNT));
    DEBUG((DEBUG_MMCHOST_SD, " - CMD_STALL_AFTER_RETRY_US=%dus\n", CMD_STALL_AFTER_RETRY_US));

    // Claim the SD0 bus (MicroSD card)
    // Configure pins 40-49 (GPFSEL4)
    {
      const UINT32 modifyMask =
        (GPIO_FSEL_MASK << (48 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |
        (GPIO_FSEL_MASK << (49 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN));

      const UINT32 functionMask =
        (GPIO_FSEL_ALT0 << (48 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |     // SD0_CLK
        (GPIO_FSEL_ALT0 << (49 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN));      // SD0_CMD

      //C_ASSERT((functionMask & ~modifyMask) == 0);

      UINT32 u32GPSEL4 = MmioRead32(GPIO_GPFSEL4);

      // Mask off to 0 the 3 function bits for these GPIO pins
      u32GPSEL4 &= ~modifyMask;

      // OR in the GPIO pins required setting
      u32GPSEL4 |= functionMask;

      MmioWrite32(GPIO_GPFSEL4, u32GPSEL4);
    } // GPFSEL4

    // Configure pins 50-53 (GPFSEL5)
    {
      const UINT32 modifyMask =
        (GPIO_FSEL_MASK << (50 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |
        (GPIO_FSEL_MASK << (51 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |
        (GPIO_FSEL_MASK << (52 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |
        (GPIO_FSEL_MASK << (53 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN));

      const UINT32 functionMask =
        (GPIO_FSEL_ALT0 << (50 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |     // SD0_DAT0
        (GPIO_FSEL_ALT0 << (51 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |     // SD0_DAT1
        (GPIO_FSEL_ALT0 << (52 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |     // SD0_DAT2
        (GPIO_FSEL_ALT0 << (53 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN));      // SD0_DAT3

      //C_ASSERT((functionMask & ~modifyMask) == 0);

      UINT32 u32GPSEL5 = MmioRead32(GPIO_GPFSEL5);

      //// Mask off to 0 the 3 function bits for these GPIO pins
      u32GPSEL5 &= ~modifyMask;

      //// OR in the GPIO pins required setting
      u32GPSEL5 |= functionMask;

      MmioWrite32(GPIO_GPFSEL5, u32GPSEL5);
    } // GPFSEL5

  
    // Route the SD1 bus to Arasan (802.11 SDIO)
    // FIXME this doesn't really belong here...
    {
      const UINT32 modifyMask =
        (GPIO_FSEL_MASK << (34 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |
        (GPIO_FSEL_MASK << (35 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |
        (GPIO_FSEL_MASK << (36 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |
        (GPIO_FSEL_MASK << (37 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |
        (GPIO_FSEL_MASK << (38 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |
        (GPIO_FSEL_MASK << (39 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN));

      const UINT32 functionMask =
        (GPIO_FSEL_ALT3 << (34 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |   // SD1_CLK
        (GPIO_FSEL_ALT3 << (35 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |   // SD1_CMD
        (GPIO_FSEL_ALT3 << (36 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |   // SD1_DAT0
        (GPIO_FSEL_ALT3 << (37 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |   // SD1_DAT1
        (GPIO_FSEL_ALT3 << (38 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN)) |   // SD1_DAT2
        (GPIO_FSEL_ALT3 << (39 % GPIO_FSEL_PINS_PER_REGISTER * GPIO_FSEL_BITS_PER_PIN));    // SD1_DAT3

      UINT32 u32GPSEL3 = MmioRead32(GPIO_GPFSEL3);

      // Mask off to 0 the 3 function bits for these GPIO pins
      u32GPSEL3 &= ~modifyMask;

      // OR in the GPIO pins for the required setting
      u32GPSEL3 |= functionMask;

      MmioWrite32(GPIO_GPFSEL3, u32GPSEL3);
    } // GPFSEL3

    Status = gBS->InstallMultipleProtocolInterfaces(
        &Handle,
        &gEfiMmcHostProtocolGuid, &gMmcHost,
        NULL
        );
    ASSERT_EFI_ERROR(Status);

    return Status;
}
