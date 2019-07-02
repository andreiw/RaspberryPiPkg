/*
 * [DSDT] Differentiated System Definition Table
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

#define BCM_ALT0 0x4
#define BCM_ALT1 0x5
#define BCM_ALT2 0x6
#define BCM_ALT3 0x7
#define BCM_ALT4 0x3
#define BCM_ALT5 0x2

DefinitionBlock ("DSDT.aml", "DSDT", 5, "MSFT", "EDK2", 2)
{
    Scope (\_SB_)
    {
        include("Sdhc.asl")
        include("PEP.asl")

        //
        // Description: This is a Processor Device
        //

        Device (CPU0)
        {
            Name (_HID, "ACPI0007")
            Name (_UID, 0x0)
            Method (_STA)
            {
                Return(0xf)
            }
        }

        //
        // Description: This is a Processor Device
        //

        Device (CPU1)
        {
            Name (_HID, "ACPI0007")
            Name (_UID, 0x1)
            Method (_STA)
            {
                Return(0xf)
            }
        }

        //
        // Description: This is a Processor Device
        //

        Device (CPU2)
        {
            Name (_HID, "ACPI0007")
            Name (_UID, 0x2)
            Method (_STA)
            {
                Return(0xf)
            }
        }

        //
        // Description: This is a Processor Device
        //

        Device (CPU3)
        {
            Name (_HID, "ACPI0007")
            Name (_UID, 0x3)
            Method (_STA)
            {
                Return(0xf)
            }
        }

        //
        // Description: DWC OTG Controller
        //

        Device (USB0)
        {
            Name (_HID, "BCM2848")
            Name (_CID, Package() { "DWC_OTG", "DWC2_OTG"})
            Name (_UID, 0x0)
            Name (_CCA, 0x0)

            Method (_STA)
            {
                Return(0xf)
            }
            Method (_CRS, 0x0, Serialized) {
                Name (RBUF, ResourceTemplate () {
                    MEMORY32FIXED(ReadWrite, 0x3F980000, 0x10000, )
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x29 }
                })
                Return(RBUF)
            }
        }

        //
        // Description: Video Core 4 GPU
        //

        Device (GPU0)
        {
            Name (_HID, "BCM2850")
            Name (_CID, "VC4")
            Name (_UID, 0x0)
            Name (_CCA, 0x0)

            Method (_STA)
            {
                Return(0xf)
            }
            Method (_CRS, 0x0, Serialized) {
                Name (RBUF, ResourceTemplate () {
                    // Memory and interrupt for the GPU
                    MEMORY32FIXED(ReadWrite, 0x3FC00000, 0x1000, )
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x2A }

                    // HVS - Hardware Video Scalar
                    MEMORY32FIXED(ReadWrite, 0x3F400000, 0x6000, )
                    // The HVS interrupt is reserved by the VPU
                    // Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x41 }

                    // PixelValve0 - DSI0 or DPI
                    // MEMORY32FIXED(ReadWrite, 0x3F206000, 0x100, )
                    // Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x4D }

                    // PixelValve1 - DS1 or SMI
                    // MEMORY32FIXED(ReadWrite, 0x73F207000, 0x100, )
                    // Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x4E }

                    // PixelValve2 - HDMI output - connected to HVS display FIFO 1
                    MEMORY32FIXED(ReadWrite, 0x3F807000, 0x100, )
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x4A }

                    // HDMI registers
                    MEMORY32FIXED(ReadWrite, 0x3F902000, 0x600, )   // HDMI registers
                    MEMORY32FIXED(ReadWrite, 0x3F808000, 0x100, )   // HD registers
                    // hdmi_int[0]
                    // Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x48 }
                    // hdmi_int[1]
                    // Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x49 }

                    // HDMI DDC connection
                    I2CSerialBus(0x50,, 100000,, "\\_SB.I2C2",,,,)  // EDID
                    I2CSerialBus(0x30,, 100000,, "\\_SB.I2C2",,,,)  // E-DDC Segment Pointer
                })
                Return(RBUF)
            }

            // GPU Power Management Component Data
            // Reference : https://github.com/Microsoft/graphics-driver-samples/wiki/Install-Driver-in-a-Windows-VM
            Method(PMCD, 0, Serialized) {

              Name(RBUF, Package()  {
                1,                  // Version
                1,                  // Number of graphics power components

                Package() {         // Power components package

                  Package() {       // GPU component package
                    0,              // Component Index
                    0,              // DXGK_POWER_COMPONENT_MAPPING.ComponentType (0 = DXGK_POWER_COMPONENT_ENGINE)
                    0,              // DXGK_POWER_COMPONENT_MAPPING.NodeIndex

                    Buffer() {      // DXGK_POWER_RUNTIME_COMPONENT.ComponentGuid
                                    // 9B2D1E26-1575-4747-8FC0-B9EB4BAA2D2B
                      0x26, 0x1E, 0x2D, 0x9B, 0x75, 0x15, 0x47, 0x47,
                      0x8f, 0xc0, 0xb9, 0xeb, 0x4b, 0xaa, 0x2d, 0x2b
                    },

                    "VC4_Engine_00",// DXGK_POWER_RUNTIME_COMPONENT.ComponentName
                    2,              // DXGK_POWER_RUNTIME_COMPONENT.StateCount

                    Package() {     // DXGK_POWER_RUNTIME_COMPONENT.States[] package

                      Package() {   // F0
                         0,         // DXGK_POWER_RUNTIME_STATE.TransitionLatency
                         0,         // DXGK_POWER_RUNTIME_STATE.ResidencyRequirement
                         1210000,   // DXGK_POWER_RUNTIME_STATE.NominalPower (microwatt)
                      },

                      Package() {   // F1 - Placeholder
                         10000,     // DXGK_POWER_RUNTIME_STATE.TransitionLatency
                         10000,     // DXGK_POWER_RUNTIME_STATE.ResidencyRequirement
                         4,         // DXGK_POWER_RUNTIME_STATE.NominalPower
                      },
                    }
                  }
                }
              })
              Return(RBUF)
            }
        }

        //
        // Description: PiQ Mailbox Driver
        //

        Device (RPIQ)
        {
            Name (_HID, "BCM2849")
            Name (_CID, "RPIQ")
            Name (_UID, 0)
            Name (_CCA, 0x0)
            Method (_STA)
            {
                Return(0xf)
            }
            Method (_CRS, 0x0, Serialized) {
                Name (RBUF, ResourceTemplate () {
                    Memory32Fixed (ReadWrite, 0x3F00B880, 0x00000024, )
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x61 }
                })
                Return (RBUF)
            }
        }

        //
        // Description: VCHIQ Driver
        //

        Device (VCIQ)
        {
            Name (_HID, "BCM2835")
            Name (_CID, "VCIQ")
            Name (_UID, 0)
            Name (_CCA, 0x0)
            Name (_DEP, Package() { \_SB.RPIQ })
            Method (_STA)
            {
                Return(0xf)
            }
            Method (_CRS, 0x0, Serialized) {
                Name (RBUF, ResourceTemplate () {
                    Memory32Fixed (ReadWrite, 0x3F00B840, 0x00000010, )
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x62 }
                })
                Return (RBUF)
            }
        }

        //
        // Description: VC Shared Memory Driver
        //

        Device (VCSM)
        {
            Name (_HID, "BCM2856")
            Name (_CID, "VCSM")
            Name (_UID, 0)
            Name (_CCA, 0x0)
            Name (_DEP, Package() { \_SB.VCIQ })
            Method (_STA)
            {
                Return(0xf)
            }
        }

        //
        // Description: GPIO
        //
        Device (GPI0)
        {
            Name (_HID, "BCM2845")
            Name (_CID, "BCMGPIO")
            Name (_UID, 0x0)
            Name (_CCA, 0x0)
            Method (_STA)
            {
                Return(0xf)
            }
            Method (_CRS, 0x0, Serialized) {
                Name (RBUF, ResourceTemplate () {
                    MEMORY32FIXED(ReadWrite, 0x3F200000, 0xB4, )
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) { 0x51 }
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) { 0x53 }
                })
                Return(RBUF)
            }
        }

        //
        // Description: I2C
        //

        Device (I2C1)
        {
            Name (_HID, "BCM2841")
            Name (_CID, "BCMI2C")
            Name (_UID, 0x1)
            Name (_CCA, 0x0)
            Method (_STA)
            {
                Return(0xf)
            }
            Method (_CRS, 0x0, Serialized)
            {
                Name (RBUF, ResourceTemplate()
                {
                    Memory32Fixed(ReadWrite, 0x3F804000, 0x20)
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) {0x55}

                    #ifdef ACPI_PINFUNCTION
                    PinFunction(Exclusive, PullUp, BCM_ALT0, "\\_SB.GPI0", 0, ResourceConsumer, , ) {2, 3}
                    #else
                    //
                    // MsftFunctionConfig is encoded as the VendorLong.
                    //
                    // MsftFunctionConfig(Exclusive, PullUp, BCM_ALT0, "\\_SB.GPI0", 0, ResourceConsumer, ) {2, 3}
                    //
                    VendorLong  ()      // Length = 0x31
                    {
                        /* 0000 */  0x00, 0x60, 0x44, 0xD5, 0xF3, 0x1F, 0x11, 0x60,  // .`D....`
                        /* 0008 */  0x4A, 0xB8, 0xB0, 0x9C, 0x2D, 0x23, 0x30, 0xDD,  // J...-#0.
                        /* 0010 */  0x2F, 0x8D, 0x1D, 0x00, 0x01, 0x10, 0x00, 0x01,  // /.......
                        /* 0018 */  0x04, 0x00, 0x12, 0x00, 0x00, 0x16, 0x00, 0x20,  // ........ 
                        /* 0020 */  0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x00, 0x5C,  // ........
                        /* 0028 */  0x5F, 0x53, 0x42, 0x2E, 0x47, 0x50, 0x49, 0x30,  // _SB.GPI0
                        /* 0030 */  0x00                                             // .
                    }
                    #endif
                })
                Return(RBUF)
            }
        }

        //
        // I2C2 is the HDMI DDC connection
        //

        Device (I2C2)
        {
            Name (_HID, "BCM2841")
            Name (_CID, "BCMI2C")
            Name (_UID, 0x2)
            Name (_CCA, 0x0)
            Method (_STA)
            {
                Return(0xf)
            }
            Method (_CRS, 0x0, Serialized)
            {
                Name (RBUF, ResourceTemplate()
                {
                    Memory32Fixed(ReadWrite, 0x3F805000, 0x20)
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) {0x55}
                })
                Return(RBUF)
            }
        }

        //
        // Description: SPI
        //

        Device (SPI0)
        {
            Name (_HID, "BCM2838")
            Name (_CID, "BCMSPI0")
            Name (_UID, 0x0)
            Name (_CCA, 0x0)
            Method (_STA)
            {
                Return(0xf)
            }
            Method (_CRS, 0x0, Serialized) {
                Name (RBUF, ResourceTemplate () {
                    MEMORY32FIXED(ReadWrite, 0x3F204000, 0x20, )
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) {0x56}

                    #ifdef ACPI_PINFUNCTION
                    PinFunction(Exclusive, PullDown, BCM_ALT0, "\\_SB.GPI0", 0, ResourceConsumer, , ) {9, 10, 11} // MISO, MOSI, SCLK
                    PinFunction(Exclusive, PullUp, BCM_ALT0, "\\_SB.GPI0", 0, ResourceConsumer, , ) {8}     // CE0
                    PinFunction(Exclusive, PullUp, BCM_ALT0, "\\_SB.GPI0", 0, ResourceConsumer, , ) {7}     // CE1
                    #else
                    //
                    // MsftFunctionConfig is encoded as the VendorLong.
                    //
                    // MsftFunctionConfig(Exclusive, PullDown, BCM_ALT0, "\\_SB.GPI0", 0, ResourceConsumer, ) {9, 10, 11} // MISO, MOSI, SCLK
                    VendorLong  ()      // Length = 0x33
                    {
                        /* 0000 */  0x00, 0x60, 0x44, 0xD5, 0xF3, 0x1F, 0x11, 0x60,  // .`D....`
                        /* 0008 */  0x4A, 0xB8, 0xB0, 0x9C, 0x2D, 0x23, 0x30, 0xDD,  // J...-#0.
                        /* 0010 */  0x2F, 0x8D, 0x1F, 0x00, 0x01, 0x10, 0x00, 0x02,  // /.......
                        /* 0018 */  0x04, 0x00, 0x12, 0x00, 0x00, 0x18, 0x00, 0x22,  // .......\"
                        /* 0020 */  0x00, 0x00, 0x00, 0x09, 0x00, 0x0A, 0x00, 0x0B,  // ........
                        /* 0028 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x50,  // .\_SB.GP
                        /* 0030 */  0x49, 0x30, 0x00                                 // I0.
                    }

                    //
                    // MsftFunctionConfig is encoded as the VendorLong.
                    //
                    // MsftFunctionConfig(Exclusive, PullUp, BCM_ALT0, "\\_SB.GPI0", 0, ResourceConsumer, ) {8}     // CE0
                    VendorLong  ()      // Length = 0x2F
                    {
                        /* 0000 */  0x00, 0x60, 0x44, 0xD5, 0xF3, 0x1F, 0x11, 0x60,  // .`D....`
                        /* 0008 */  0x4A, 0xB8, 0xB0, 0x9C, 0x2D, 0x23, 0x30, 0xDD,  // J...-#0.
                        /* 0010 */  0x2F, 0x8D, 0x1B, 0x00, 0x01, 0x10, 0x00, 0x01,  // /.......
                        /* 0018 */  0x04, 0x00, 0x12, 0x00, 0x00, 0x14, 0x00, 0x1E,  // ........
                        /* 0020 */  0x00, 0x00, 0x00, 0x08, 0x00, 0x5C, 0x5F, 0x53,  // .....\_S
                        /* 0028 */  0x42, 0x2E, 0x47, 0x50, 0x49, 0x30, 0x00         // B.GPI0.
                    }

                    //
                    // MsftFunctionConfig is encoded as the VendorLong.
                    //
                    // MsftFunctionConfig(Exclusive, PullUp, BCM_ALT0, "\\_SB.GPI0", 0, ResourceConsumer, ) {7}     // CE1
                    VendorLong  ()      // Length = 0x2F
                    {
                        /* 0000 */  0x00, 0x60, 0x44, 0xD5, 0xF3, 0x1F, 0x11, 0x60,  // .`D....`
                        /* 0008 */  0x4A, 0xB8, 0xB0, 0x9C, 0x2D, 0x23, 0x30, 0xDD,  // J...-#0.
                        /* 0010 */  0x2F, 0x8D, 0x1B, 0x00, 0x01, 0x10, 0x00, 0x01,  // /.......
                        /* 0018 */  0x04, 0x00, 0x12, 0x00, 0x00, 0x14, 0x00, 0x1E,  // ........
                        /* 0020 */  0x00, 0x00, 0x00, 0x07, 0x00, 0x5C, 0x5F, 0x53,  // .....\_S
                        /* 0028 */  0x42, 0x2E, 0x47, 0x50, 0x49, 0x30, 0x00         // B.GPI0.
                    }
                    #endif
                })
                Return(RBUF)
            }
        }

        Device (SPI1)
        {
            Name (_HID, "BCM2839")
            Name (_CID, "BCMAUXSPI")
            Name (_UID, 0x1)
            Name (_CCA, 0x0)
            Name (_DEP, Package() { \_SB.RPIQ })
            Method (_STA)
            {
                Return(0xf)
            }
            Method (_CRS, 0x0, Serialized) {
                Name (RBUF, ResourceTemplate () {
                    MEMORY32FIXED(ReadWrite, 0x3F215080, 0x40,)
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Shared,) {0x3D}

                    #ifdef ACPI_PINFUNCTION
                    PinFunction(Exclusive, PullDown, BCM_ALT4, "\\_SB.GPI0", 0, ResourceConsumer, , ) {19, 20, 21} // MISO, MOSI, SCLK
                    PinFunction(Exclusive, PullDown, BCM_ALT4, "\\_SB.GPI0", 0, ResourceConsumer, , ) {16} // CE2
                    #else
                    //
                    // MsftFunctionConfig is encoded as the VendorLong.
                    //
                    // MsftFunctionConfig(Exclusive, PullDown, BCM_ALT4, "\\_SB.GPI0", 0, ResourceConsumer, ) {19, 20, 21} // MISO, MOSI, SCLK
                    VendorLong  ()      // Length = 0x33
                    {
                        /* 0000 */  0x00, 0x60, 0x44, 0xD5, 0xF3, 0x1F, 0x11, 0x60,  // .`D....`
                        /* 0008 */  0x4A, 0xB8, 0xB0, 0x9C, 0x2D, 0x23, 0x30, 0xDD,  // J...-#0.
                        /* 0010 */  0x2F, 0x8D, 0x1F, 0x00, 0x01, 0x10, 0x00, 0x02,  // /.......
                        /* 0018 */  0x03, 0x00, 0x12, 0x00, 0x00, 0x18, 0x00, 0x22,  // .......\"
                        /* 0020 */  0x00, 0x00, 0x00, 0x13, 0x00, 0x14, 0x00, 0x15,  // ........
                        /* 0028 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x50,  // .\_SB.GP
                        /* 0030 */  0x49, 0x30, 0x00                                 // I0.
                    }

                    //
                    // MsftFunctionConfig is encoded as the VendorLong.
                    //
                    // MsftFunctionConfig(Exclusive, PullDown, BCM_ALT4, "\\_SB.GPI0", 0, ResourceConsumer, ) {16} // CE2
                    VendorLong  ()      // Length = 0x2F
                    {
                        /* 0000 */  0x00, 0x60, 0x44, 0xD5, 0xF3, 0x1F, 0x11, 0x60,  // .`D....`
                        /* 0008 */  0x4A, 0xB8, 0xB0, 0x9C, 0x2D, 0x23, 0x30, 0xDD,  // J...-#0.
                        /* 0010 */  0x2F, 0x8D, 0x1B, 0x00, 0x01, 0x10, 0x00, 0x02,  // /.......
                        /* 0018 */  0x03, 0x00, 0x12, 0x00, 0x00, 0x14, 0x00, 0x1E,  // ........
                        /* 0020 */  0x00, 0x00, 0x00, 0x10, 0x00, 0x5C, 0x5F, 0x53,  // .....\_S
                        /* 0028 */  0x42, 0x2E, 0x47, 0x50, 0x49, 0x30, 0x00         // B.GPI0.
                    }
                    #endif
                })
                Return(RBUF)
            }
        }

        // SPI2 has no pins on GPIO header
        // Device (SPI2)
        // {
            // Name (_HID, "BCM2839")
            // Name (_CID, "BCMAUXSPI")
            // Name (_UID, 0x2)
            // Name (_CCA, 0x0)
            // Name (_DEP, Package() { \_SB.RPIQ })
            // Method (_STA)
           // {
                // Return(0xf)     // Disabled
            // }
            // Method (_CRS, 0x0, Serialized) {
                // Name (RBUF, ResourceTemplate () {
                    // MEMORY32FIXED(ReadWrite, 0x3F2150C0, 0x40,)
                    // Interrupt(ResourceConsumer, Level, ActiveHigh, Shared,) {0x3D}
                // })
                // Return(RBUF)
            // }
        // }

        //
        // Description: PWM Driver
        //

        Device (PWM0)
        {
            Name (_HID, "BCM2844")
            Name (_CID, "BCM2844")
            Name (_UID, 0)
            Name (_CCA, 0x0)
            Method (_STA)
            {
                Return(0xf)
            }
            Method (_CRS, 0x0, Serialized) {
                Name (RBUF, ResourceTemplate () {
                    // DMA channel 11 control
                    Memory32Fixed (ReadWrite, 0x3F007B00, 0x00000100, )
                    // PWM control
                    Memory32Fixed (ReadWrite, 0x3F20C000, 0x00000028, )
                    // PWM control bus
                    Memory32Fixed (ReadWrite, 0x7E20C000, 0x00000028, )
                    // PWM control uncached
                    Memory32Fixed (ReadWrite, 0xFF20C000, 0x00000028, )
                    // PWM clock control
                    Memory32Fixed (ReadWrite, 0x3F1010A0, 0x00000008, )
                    // Interrupt DMA channel 11
                    Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x3B }
                    // DMA channel 11, DREQ 5 for PWM
                    FixedDMA(5, 11, Width32Bit, )
                })
                Return (RBUF)
            }
        }

        include("Uart.asl")
        include("Rhpx.asl")
    }
}
