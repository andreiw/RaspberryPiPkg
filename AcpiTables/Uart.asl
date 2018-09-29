//
// Description: This is the PL011 based UART
//

Device (URT0)
{
    Name (_HID, "BCM2837")
    Name (_CID, "HID3123")
    Name (_UID, 0x4)
    Method (_STA)
    {
        Return(0xf)
    }
    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate () {
            MEMORY32FIXED(ReadWrite, 0x3F201000, 0x1000, )
            Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 0x59 }
        })
        Return(RBUF)
    }

    Name (CLCK, 3000000)

    Name (_DSD, Package () {
        ToUUID("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
            Package () {
                Package (2) { "clock-frequency", CLCK },
            }
        })
}

//
// Description: UART Mini
//
// This device is referenced in the DBG2 table, which will cause the system to
// not start the driver when the debugger is enabled and to mark the device
// with problem code 53 (CM_PROB_USED_BY_DEBUGGER).
//

Device (URTM)
{
    Name (_HID, "BCM2836")
    Name (_CID, "MINIUART")
    Name (_UID, 0x0)
    Method (_STA)
    {
        Return(0xf)
    }
    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate () {
            MEMORY32FIXED(ReadWrite, 0x3F215000, 0x70, )
            Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) {0x3D}

            // NTRAID#MSFT-7141401-2016/04/7-jordanrh - disable UART muxing
            // until a proper solution can be created for the dmap conflict.
            // When muxing is enabled, must consider DBG2 table conflict.
            // The alternate function resource needs to be reserved when
            // the kernel debugger is enabled to prevent another client
            // from muxing the pins away.

            //
            // MsftFunctionConfig is encoded as the VendorLong.
            //
            // MsftFunctionConfig(Exclusive, PullDown, BCM_ALT5, "\\_SB.GPI0", 0, ResourceConsumer, ) {14, 15}
            // VendorLong  ()      // Length = 0x31
            // {
            //    /* 0000 */  0x00, 0x60, 0x44, 0xD5, 0xF3, 0x1F, 0x11, 0x60,  // .`D....`
            //    /* 0008 */  0x4A, 0xB8, 0xB0, 0x9C, 0x2D, 0x23, 0x30, 0xDD,  // J...-#0.
            //    /* 0010 */  0x2F, 0x8D, 0x1D, 0x00, 0x01, 0x10, 0x00, 0x02,  // /.......
            //    /* 0018 */  0x02, 0x00, 0x12, 0x00, 0x00, 0x16, 0x00, 0x20,  // ....... 
            //    /* 0020 */  0x00, 0x00, 0x00, 0x0E, 0x00, 0x0F, 0x00, 0x5C,  // .......\
            //    /* 0028 */  0x5F, 0x53, 0x42, 0x2E, 0x47, 0x50, 0x49, 0x30,  // _SB.GPI0
            //    /* 0030 */  0x00                                             // .
            //}

        })
        Return(RBUF)
    }
}

//
// Multifunction serial bus device to support Bluetooth function
//

Device(BTH0)
{
    Name (_HID, "BCM2EA6")
    Name (_CID, "BCM2EA6")
    Method (_STA)
    {
        Return(0xf)
    }
    Method (_CRS, 0x0, Serialized) {
        Name (RBUF, ResourceTemplate () {
            //
            // BT UART: UART0 (PL011)
            //
            UARTSerialBus(
                115200,        // InitialBaudRate: in BPS
                ,              // BitsPerByte: default to 8 bits
                ,              // StopBits: Defaults to one bit
                0x00,          // LinesInUse: 8 1-bit flags to
                               //   declare enabled control lines.
                               //   Raspberry Pi does not exposed
                               //   HW control signals -> not supported.
                               //   Optional bits:
                               //   - Bit 7 (0x80) Request To Send (RTS)
                               //   - Bit 6 (0x40) Clear To Send (CTS)
                               //   - Bit 5 (0x20) Data Terminal Ready (DTR)
                               //   - Bit 4 (0x10) Data Set Ready (DSR)
                               //   - Bit 3 (0x08) Ring Indicator (RI)
                               //   - Bit 2 (0x04) Data Carrier Detect (DTD)
                               //   - Bit 1 (0x02) Reserved. Must be 0.
                               //   - Bit 0 (0x01) Reserved. Must be 0.
                ,              // IsBigEndian:
                               //   default to LittleEndian.
                ,              // Parity: Defaults to no parity
                ,              // FlowControl: Defaults to
                               //   no flow control.
                16,            // ReceiveBufferSize
                16,            // TransmitBufferSize
                "\\_SB.URT0",  // ResourceSource:
                               //   UART bus controller name
                ,              // ResourceSourceIndex: assumed to be 0
                ,              // ResourceUsage: assumed to be
                               //   ResourceConsumer
                UAR0,          // DescriptorName: creates name
                               //   for offset of resource descriptor
                )              // Vendor data

               //
               // RPIQ connection for BT_ON/OFF
               //
               GpioIO(Shared, PullUp, 0, 0, IoRestrictionNone, "\\_SB.RPIQ", 0, ResourceConsumer, , ) { 128 }
        })
        Return(RBUF)
    }
}
