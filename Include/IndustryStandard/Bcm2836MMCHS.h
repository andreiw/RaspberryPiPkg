/** @file
*
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

#ifndef __BCM2836SDIO_H__
#define __BCM2836SDIO_H__

//MMC/SD/SDIO1 register definitions.
#define MMCHS1BASE        0x3F300000

#define MMCHS_BLK         (MMCHS1BASE + 0x4)
#define BLEN_512BYTES     (0x200UL << 0)

#define MMCHS_ARG         (MMCHS1BASE + 0x8)

#define MMCHS_CMD         (MMCHS1BASE + 0xC)
#define BCE_ENABLE        BIT1
#define DDIR_READ         BIT4
#define DDIR_WRITE        (0x0UL << 4)
#define MSBS_SGLEBLK      (0x0UL << 5)
#define MSBS_MULTBLK      BIT5
#define RSP_TYPE_MASK     (0x3UL << 16)
#define RSP_TYPE_136BITS  BIT16
#define RSP_TYPE_48BITS   (0x2UL << 16)
#define RSP_TYPE_48BUSY   (0x3UL << 16)
#define CCCE_ENABLE       BIT19
#define CICE_ENABLE       BIT20
#define DP_ENABLE         BIT21
#define INDX(CMD_INDX)    ((CMD_INDX & 0x3F) << 24)

#define MMCHS_RSP10       (MMCHS1BASE + 0x10)
#define MMCHS_RSP32       (MMCHS1BASE + 0x14)
#define MMCHS_RSP54       (MMCHS1BASE + 0x18)
#define MMCHS_RSP76       (MMCHS1BASE + 0x1C)
#define MMCHS_DATA        (MMCHS1BASE + 0x20)

#define MMCHS_PRES_STATE  (MMCHS1BASE + 0x24)
#define CMDI_MASK         BIT0
#define CMDI_ALLOWED      (0x0UL << 0)
#define CMDI_NOT_ALLOWED  BIT0
#define DATI_MASK         BIT1
#define DATI_ALLOWED      (0x0UL << 1)
#define DATI_NOT_ALLOWED  BIT1
#define WRITE_PROTECT_OFF BIT19

#define MMCHS_HCTL        (MMCHS1BASE + 0x28)
#define DTW_1_BIT         (0x0UL << 1)
#define DTW_4_BIT         BIT1
#define SDBP_MASK         BIT8
#define SDBP_OFF          (0x0UL << 8)
#define SDBP_ON           BIT8
#define SDVS_1_8_V        (0x5UL << 9)
#define SDVS_3_0_V        (0x6UL << 9)
#define IWE               BIT24

#define MMCHS_SYSCTL      (MMCHS1BASE + 0x2C)
#define ICE               BIT0
#define ICS_MASK          BIT1
#define ICS               BIT1
#define CEN               BIT2
#define CLKD_MASK         (0x3FFUL << 6)
#define CLKD_80KHZ        (0x258UL) //(96*1000/80)/2
#define CLKD_400KHZ       (0xF0UL)
#define CLKD_12500KHZ     (0x200UL)
#define DTO_MASK          (0xFUL << 16)
#define DTO_VAL           (0xEUL << 16)
#define SRA               BIT24
#define SRC_MASK          BIT25
#define SRC               BIT25
#define SRD               BIT26

#define MMCHS_INT_STAT    (MMCHS1BASE + 0x30)
#define CC                BIT0
#define TC                BIT1
#define BWR               BIT4
#define BRR               BIT5
#define CARD_INS          BIT6
#define ERRI              BIT15
#define CTO               BIT16
#define DTO               BIT20
#define DCRC              BIT21
#define DEB               BIT22

#define MMCHS_IE          (MMCHS1BASE + 0x34)
#define CC_EN             BIT0
#define TC_EN             BIT1
#define BWR_EN            BIT4
#define BRR_EN            BIT5
#define CTO_EN            BIT16
#define CCRC_EN           BIT17
#define CEB_EN            BIT18
#define CIE_EN            BIT19
#define DTO_EN            BIT20
#define DCRC_EN           BIT21
#define DEB_EN            BIT22
#define CERR_EN           BIT28
#define BADA_EN           BIT29
#define ALL_EN            0xFFFFFFFF

#define MMCHS_ISE         (MMCHS1BASE + 0x38)
#define CC_SIGEN          BIT0
#define TC_SIGEN          BIT1
#define BWR_SIGEN         BIT4
#define BRR_SIGEN         BIT5
#define CTO_SIGEN         BIT16
#define CCRC_SIGEN        BIT17
#define CEB_SIGEN         BIT18
#define CIE_SIGEN         BIT19
#define DTO_SIGEN         BIT20
#define DCRC_SIGEN        BIT21
#define DEB_SIGEN         BIT22
#define CERR_SIGEN        BIT28
#define BADA_SIGEN        BIT29

#define MMCHS_AC12        (MMCHS1BASE + 0x3C)

#define MMCHS_CAPA        (MMCHS1BASE + 0x40)
#define VS30              BIT25
#define VS18              BIT26

#define MMCHS_CUR_CAPA    (MMCHS1BASE + 0x48)
#define MMCHS_REV         (MMCHS1BASE + 0xFC)

#define BLOCK_COUNT_SHIFT 16
#define RCA_SHIFT         16

#define CMD_R1            RSP_TYPE_48BITS | CCCE_ENABLE | CICE_ENABLE
#define CMD_R1B           RSP_TYPE_48BUSY | CCCE_ENABLE | CICE_ENABLE
#define CMD_R2            RSP_TYPE_136BITS | CCCE_ENABLE
#define CMD_R3            RSP_TYPE_48BITS
#define CMD_R6            RSP_TYPE_48BITS | CCCE_ENABLE | CICE_ENABLE
#define CMD_R7            RSP_TYPE_48BITS | CCCE_ENABLE | CICE_ENABLE

#define CMD_R1_ADTC       CMD_R1 | DP_ENABLE
#define CMD_R1_ADTC_READ  CMD_R1_ADTC | DDIR_READ
#define CMD_R1_ADTC_WRITE CMD_R1_ADTC | DDIR_WRITE


#define CMD0              (INDX(0)) // Go idle
#define CMD1              (INDX(1) | CMD_R3) // MMC: Send Op Cond
#define CMD2              (INDX(2) | CMD_R2) // Send CID
#define CMD3              (INDX(3) | CMD_R6) // Set Relative Addr
#define CMD4              (INDX(4)) // Set DSR
#define CMD5              (INDX(5) | CMD_R1B) // SDIO: Sleep/Awake
#define CMD6              (INDX(6) | CMD_R1_ADTC_READ) // Switch
#define CMD7              (INDX(7) | CMD_R1B) // Select/Deselect
#define CMD8              (INDX(8) | CMD_R7) // Send If Cond
#define CMD8_ARG          (0x0UL << 12 | BIT8 | 0xCEUL << 0)
#define CMD9              (INDX(9) | CMD_R2) // Send CSD
#define CMD10             (INDX(10) | CMD_R2) // Send CID
#define CMD11             (INDX(11) | CMD_R1) // Voltage Switch
#define CMD12             (INDX(12) | CMD_R1B) // Stop Transmission
#define CMD13             (INDX(13) | CMD_R1) // Send Status
#define CMD15             (INDX(15)) // Go inactive state
#define CMD16             (INDX(16) | CMD_R1) // Set Blocklen
#define CMD17             (INDX(17) | CMD_R1_ADTC_READ) // Read Single Block
#define CMD18             (INDX(18) | CMD_R1_ADTC_READ | MSBS_MULTBLK | BCE_ENABLE) // Read Multiple Blocks
#define CMD19             (INDX(19) | CMD_R1_ADTC_READ) // SD: Send Tuning Block (64 bytes)
#define CMD20             (INDX(20) | CMD_R1B) // SD: Speed Class Control
#define CMD23             (INDX(23) | CMD_R1) // Set Block Count for CMD18 and CMD25
#define CMD24             (INDX(24) | CMD_R1_ADTC_WRITE) // Write Block
#define CMD25             (INDX(25) | CMD_R1_ADTC_WRITE | MSBS_MULTBLK | BCE_ENABLE) // Write Multiple Blocks
#define CMD55             (INDX(55) | CMD_R1) // App Cmd

#define ACMD6             (INDX(6) | CMD_R1) // Set Bus Width
#define ACMD41            (INDX(41) | CMD_R3) // Send Op Cond
#define ACMD51            (INDX(51) | CMD_R1_ADTC_READ) // Send SCR

// User-friendly command names
#define CMD_IO_SEND_OP_COND      CMD5
#define CMD_SEND_CSD             CMD9  // CSD: Card-Specific Data
#define CMD_STOP_TRANSMISSION    CMD12
#define CMD_READ_SINGLE_BLOCK    CMD17
#define CMD_READ_MULTIPLE_BLOCK  CMD18
#define CMD_SET_BLOCK_COUNT      CMD23
#define CMD_WRITE_SINGLE_BLOCK   CMD24
#define CMD_WRITE_MULTIPLE_BLOCK CMD25

#endif //__BCM2836SDIO_H__
