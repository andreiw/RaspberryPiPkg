/*
 * [DSDT] SD controller/card definition.
 *
 * UEFI can use either SDHost or Arasan. We expose both to HLOS.
 *
 * Copyright (c), 2018, Andrey Warkentin <andrey.warkentin@gmail.com>
 * Copyright (c), Microsoft Corporation. All rights reserved.
 *
 * This program and the accompanying materials
 * are licensed and made available under the terms and conditions of the BSD License
 * which accompanies this distribution.  The full text of the license may be found at
 * http://opensource.org/licenses/bsd-license.php
 *
 * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 */

//
// Description: This is ArasanSD 3.0 SD Host Controller.
//

Device (SDC1)
{
    Name (_HID, "BCM2847")
    Name (_CID, "ARASAN")
    Name (_UID, 0x0)
    Name (_S1D, 0x1)
    Name (_S2D, 0x1)
    Name (_S3D, 0x1)
    Name (_S4D, 0x1)
    Method (_STA)
    {
        Return(0xf)
    }
    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate () {
            MEMORY32FIXED(ReadWrite, 0x3F300000, 0x100, )
            Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x5E }
        })
        Return(RBUF)
    }

    //
    // A child device that represents the
    // sd card, which is marked as non-removable.
    //
    Device (SDMM)
    {
        Method (_ADR)
        {
            Return (0)
        }
        Method (_RMV) // Is removable
        {
            Return (0) // 0 - fixed
        }
    }
}


//
// Description: This is Broadcom SDHost 2.0 SD Host Controller
//

Device (SDC2)
{
    Name (_HID, "BCM2855")
    Name (_CID, "SDHST")
    Name (_UID, 0x0)
    Name (_S1D, 0x1)
    Name (_S2D, 0x1)
    Name (_S3D, 0x1)
    Name (_S4D, 0x1)
    Method (_STA)
    {
        Return(0xf)
    }
    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate () {
            MEMORY32FIXED(ReadWrite, 0x3F202000, 0x100, )
            Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x58 }
        })
        Return(RBUF)
    }

    //
    // A child device that represents the
    // sd card, which is marked as non-removable.
    // 
    Device (SDMM)
    {
        Method (_ADR)
        {
            Return (0)
        }
        Method (_RMV) // Is removable
        {
            Return (0) // 0 - fixed
        }
    }
}