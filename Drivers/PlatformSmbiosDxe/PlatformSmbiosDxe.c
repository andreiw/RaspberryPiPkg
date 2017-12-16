/** @file
  Static SMBIOS Table for ARM platform
  Derived from EmulatorPkg package

  Note SMBIOS 2.7.1 Required structures:
    BIOS Information (Type 0)
    System Information (Type 1)
    Board Information (Type 2)
    System Enclosure (Type 3)
    Processor Information (Type 4) - CPU Driver
    Cache Information (Type 7) - For cache that is external to processor
    System Slots (Type 9) - If system has slots
    Physical Memory Array (Type 16)
    Memory Device (Type 17) - For each socketed system-memory Device
    Memory Array Mapped Address (Type 19) - One per contiguous block per Physical Memroy Array
    System Boot Information (Type 32)


  Copyright (c), 2017, Andrey Warkentin <andrey.warkentin@gmail.com>
  Copyright (c), Microsoft Corporation. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


  Copyright (c) 2012, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2013 Linaro.org
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Protocol/Smbios.h>
#include <Protocol/RaspberryPiFirmware.h>
#include <IndustryStandard/SmBios.h>
#include <IndustryStandard/RpiFirmware.h>
#include <Guid/SmBios.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC RASPBERRY_PI_FIRMWARE_PROTOCOL *mFwProtocol;

/***********************************************************************
	SMBIOS data definition  TYPE0  BIOS Information
************************************************************************/
SMBIOS_TABLE_TYPE0 mBIOSInfoType0 = {
  { EFI_SMBIOS_TYPE_BIOS_INFORMATION, sizeof (SMBIOS_TABLE_TYPE0), 0 },
  1,                    // Vendor String
  2,                    // BiosVersion String
  0xE000,               // BiosSegment
  3,                    // BiosReleaseDate String
  0x7F,                 // BiosSize
  {                     // BiosCharacteristics
    0,    //  Reserved                          :2;  ///< Bits 0-1.
    0,    //  Unknown                           :1;
    0,    //  BiosCharacteristicsNotSupported   :1;
    0,    //  IsaIsSupported                    :1;
    0,    //  McaIsSupported                    :1;
    0,    //  EisaIsSupported                   :1;
    0,    //  PciIsSupported                    :1;
    0,    //  PcmciaIsSupported                 :1;
    0,    //  PlugAndPlayIsSupported            :1;
    0,    //  ApmIsSupported                    :1;
    1,    //  BiosIsUpgradable                  :1;
    1,    //  BiosShadowingAllowed              :1;
    0,    //  VlVesaIsSupported                 :1;
    0,    //  EscdSupportIsAvailable            :1;
    0,    //  BootFromCdIsSupported             :1;
    1,    //  SelectableBootIsSupported         :1;
    0,    //  RomBiosIsSocketed                 :1;
    0,    //  BootFromPcmciaIsSupported         :1;
    0,    //  EDDSpecificationIsSupported       :1;
    0,    //  JapaneseNecFloppyIsSupported      :1;
    0,    //  JapaneseToshibaFloppyIsSupported  :1;
    0,    //  Floppy525_360IsSupported          :1;
    0,    //  Floppy525_12IsSupported           :1;
    0,    //  Floppy35_720IsSupported           :1;
    0,    //  Floppy35_288IsSupported           :1;
    0,    //  PrintScreenIsSupported            :1;
    0,    //  Keyboard8042IsSupported           :1;
    0,    //  SerialIsSupported                 :1;
    0,    //  PrinterIsSupported                :1;
    0,    //  CgaMonoIsSupported                :1;
    0,    //  NecPc98                           :1;
    0     //  ReservedForVendor                 :32; ///< Bits 32-63. Bits 32-47 reserved for BIOS vendor
                                                 ///< and bits 48-63 reserved for System Vendor.
  },
  {       // BIOSCharacteristicsExtensionBytes[]
    0x81, //  AcpiIsSupported                   :1;
          //  UsbLegacyIsSupported              :1;
          //  AgpIsSupported                    :1;
          //  I2OBootIsSupported                :1;
          //  Ls120BootIsSupported              :1;
          //  AtapiZipDriveBootIsSupported      :1;
          //  Boot1394IsSupported               :1;
          //  SmartBatteryIsSupported           :1;
          //  BIOSCharacteristicsExtensionBytes[1]
    0x0e, //  BiosBootSpecIsSupported              :1;
          //  FunctionKeyNetworkBootIsSupported    :1;
          //  TargetContentDistributionEnabled     :1;
          //  UefiSpecificationSupported           :1;
          //  VirtualMachineSupported              :1;
          //  ExtensionByte2Reserved               :3;
  },
  0x00,                    // SystemBiosMajorRelease
  0x01,                    // SystemBiosMinorRelease
  0xFF,                    // EmbeddedControllerFirmwareMajorRelease
  0xFF,                    // EmbeddedControllerFirmwareMinorRelease
};

CHAR8 *mBIOSInfoType0Strings[] = {
  "Raspberry Pi 3 64-bit UEFI", // Vendor String
  "Built: " __DATE__,             // BiosVersion String
  "Built: " __DATE__,             // BiosReleaseDate String
  NULL
};

/***********************************************************************
	SMBIOS data definition  TYPE1  System Information
************************************************************************/
SMBIOS_TABLE_TYPE1 mSysInfoType1 = {
  { EFI_SMBIOS_TYPE_SYSTEM_INFORMATION, sizeof (SMBIOS_TABLE_TYPE1), 0 },
  1,    // Manufacturer String
  2,    // ProductName String
  3,    // Version String
  4,    // SerialNumber String
  { 0x25EF0280, 0xEC82, 0x42B0, { 0x8F, 0xB6, 0x10, 0xAD, 0xCC, 0xC6, 0x7C, 0x02 } },
  SystemWakeupTypePowerSwitch,
  5,    // SKUNumber String
  6,    // Family String
};
CHAR8  *mSysInfoType1Strings[] = {
  "Raspberry Pi",
  "Raspberry Pi 3",
  "1.0",
  "System Serial#",
  "RPi3-1GB",
  "edk2",
  NULL
};

/***********************************************************************
	SMBIOS data definition  TYPE2  Board Information
************************************************************************/
SMBIOS_TABLE_TYPE2  mBoardInfoType2 = {
  { EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION, sizeof (SMBIOS_TABLE_TYPE2), 0 },
  1,    // Manufacturer String
  2,    // ProductName String
  3,    // Version String
  4,    // SerialNumber String
  5,    // AssetTag String
  {     // FeatureFlag
    1,    //  Motherboard           :1;
    0,    //  RequiresDaughterCard  :1;
    0,    //  Removable             :1;
    0,    //  Replaceable           :1;
    0,    //  HotSwappable          :1;
    0,    //  Reserved              :3;
  },
  6,    // LocationInChassis String
  0,                        // ChassisHandle;
  BaseBoardTypeMotherBoard, // BoardType;
  0,                        // NumberOfContainedObjectHandles;
  { 0 }                     // ContainedObjectHandles[1];
};
CHAR8  *mBoardInfoType2Strings[] = {
  "Raspberry Pi",
  "Raspberry Pi 3",
  "1.0",
  "Base Board Serial#",
  "Base Board Asset Tag#",
  "Part Component",
  NULL
};

/***********************************************************************
	SMBIOS data definition  TYPE3  Enclosure Information
************************************************************************/
SMBIOS_TABLE_TYPE3  mEnclosureInfoType3 = {
  { EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE, sizeof (SMBIOS_TABLE_TYPE3), 0 },
  1,                        // Manufacturer String
  MiscChassisTypeLapTop,    // Type;
  2,                        // Version String
  3,                        // SerialNumber String
  4,                        // AssetTag String
  ChassisStateSafe,         // BootupState;
  ChassisStateSafe,         // PowerSupplyState;
  ChassisStateSafe,         // ThermalState;
  ChassisSecurityStatusNone,// SecurityStatus;
  { 0, 0, 0, 0 },           // OemDefined[4];
  0,    // Height;
  0,    // NumberofPowerCords;
  0,    // ContainedElementCount;
  0,    // ContainedElementRecordLength;
  { { 0 } },    // ContainedElements[1];
};
CHAR8  *mEnclosureInfoType3Strings[] = {
  "Raspberry Pi",
  "1.0",
  "Chassis Board Serial#",
  "Chassis Board Asset Tag#",
  NULL
};

/***********************************************************************
	SMBIOS data definition  TYPE4  Processor Information
************************************************************************/
SMBIOS_TABLE_TYPE4 mProcessorInfoType4 = {
  { EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE4), 0},
  1,                    // Socket String
  CentralProcessor,       // ProcessorType;				      ///< The enumeration value from PROCESSOR_TYPE_DATA.
  ProcessorFamilyIndicatorFamily2, // ProcessorFamily;        ///< The enumeration value from PROCESSOR_FAMILY2_DATA.
  2,                    // ProcessorManufacture String;
  {                     // ProcessorId;
    {  // PROCESSOR_SIGNATURE
      0, //  ProcessorSteppingId:4;
      0, //  ProcessorModel:     4;
      0, //  ProcessorFamily:    4;
      0, //  ProcessorType:      2;
      0, //  ProcessorReserved1: 2;
      0, //  ProcessorXModel:    4;
      0, //  ProcessorXFamily:   8;
      0, //  ProcessorReserved2: 4;
    },

    {  // PROCESSOR_FEATURE_FLAGS
      0, //  ProcessorFpu       :1;
      0, //  ProcessorVme       :1;
      0, //  ProcessorDe        :1;
      0, //  ProcessorPse       :1;
      0, //  ProcessorTsc       :1;
      0, //  ProcessorMsr       :1;
      0, //  ProcessorPae       :1;
      0, //  ProcessorMce       :1;
      0, //  ProcessorCx8       :1;
      0, //  ProcessorApic      :1;
      0, //  ProcessorReserved1 :1;
      0, //  ProcessorSep       :1;
      0, //  ProcessorMtrr      :1;
      0, //  ProcessorPge       :1;
      0, //  ProcessorMca       :1;
      0, //  ProcessorCmov      :1;
      0, //  ProcessorPat       :1;
      0, //  ProcessorPse36     :1;
      0, //  ProcessorPsn       :1;
      0, //  ProcessorClfsh     :1;
      0, //  ProcessorReserved2 :1;
      0, //  ProcessorDs        :1;
      0, //  ProcessorAcpi      :1;
      0, //  ProcessorMmx       :1;
      0, //  ProcessorFxsr      :1;
      0, //  ProcessorSse       :1;
      0, //  ProcessorSse2      :1;
      0, //  ProcessorSs        :1;
      0, //  ProcessorReserved3 :1;
      0, //  ProcessorTm        :1;
      0, //  ProcessorReserved4 :2;
    }
  },
  3,                    // ProcessorVersion String;
  {                     // Voltage;
    1,  // ProcessorVoltageCapability5V        :1;
    1,  // ProcessorVoltageCapability3_3V      :1;
    1,  // ProcessorVoltageCapability2_9V      :1;
    0,  // ProcessorVoltageCapabilityReserved  :1; ///< Bit 3, must be zero.
    0,  // ProcessorVoltageReserved            :3; ///< Bits 4-6, must be zero.
    0   // ProcessorVoltageIndicateLegacy      :1;
  },
  0,                      // ExternalClock;
  0,                      // MaxSpeed;
  0,                      // CurrentSpeed;
  0x41,                   // Status;
  ProcessorUpgradeOther,  // ProcessorUpgrade;      ///< The enumeration value from PROCESSOR_UPGRADE.
  0,                      // L1CacheHandle;
  0,                      // L2CacheHandle;
  0,                      // L3CacheHandle;
  4,                      // SerialNumber;
  5,                      // AssetTag;
  6,                      // PartNumber;
  4,                      // CoreCount;
  4,                      // EnabledCoreCount;
  4,                      // ThreadCount;
  0x6C,                   // ProcessorCharacteristics;
  ProcessorFamilyARM,     // ARM Processor Family;
};

CHAR8 *mProcessorInfoType4Strings[] = {
  "Socket",
  "ARM",
  "BCM2837 ARMv8",
  "1.0",
  "1.0",
  "1.0",
  NULL
};

/***********************************************************************
	SMBIOS data definition  TYPE7  Cache Information
************************************************************************/
SMBIOS_TABLE_TYPE7  mCacheInfoType7 = {
  { EFI_SMBIOS_TYPE_CACHE_INFORMATION, sizeof (SMBIOS_TABLE_TYPE7), 0 },
  1,                        // SocketDesignation String
  0x018A,					// Cache Configuration
  0x00FF,					// Maximum Size 256k
  0x00FF,					// Install Size 256k
  {                         // Supported SRAM Type
	0,  //Other             :1
	0,  //Unknown           :1
	0,  //NonBurst          :1
	1,  //Burst             :1
	0,  //PiplelineBurst    :1
	1,  //Synchronous       :1
	0,  //Asynchronous      :1
	0	//Reserved          :9
  },
  {                         // Current SRAM Type
	0,  //Other             :1
	0,  //Unknown           :1
	0,  //NonBurst          :1
	1,  //Burst             :1
	0,  //PiplelineBurst    :1
	1,  //Synchronous       :1
	0,  //Asynchronous      :1
	0	//Reserved          :9
  },
  0,						// Cache Speed unknown
  CacheErrorMultiBit,		// Error Correction Multi
  CacheTypeUnknown,			// System Cache Type
  CacheAssociativity2Way	// Associativity
};
CHAR8  *mCacheInfoType7Strings[] = {
  "Cache1",
  NULL
};

/***********************************************************************
	SMBIOS data definition  TYPE9  System Slot Information
************************************************************************/
SMBIOS_TABLE_TYPE9  mSysSlotInfoType9 = {
  { EFI_SMBIOS_TYPE_SYSTEM_SLOTS, sizeof (SMBIOS_TABLE_TYPE9), 0 },
  1,    // SlotDesignation String
  SlotTypeOther,          // SlotType;                 ///< The enumeration value from MISC_SLOT_TYPE.
  SlotDataBusWidthOther,  // SlotDataBusWidth;         ///< The enumeration value from MISC_SLOT_DATA_BUS_WIDTH.
  SlotUsageAvailable,    // CurrentUsage;             ///< The enumeration value from MISC_SLOT_USAGE.
  SlotLengthOther,    // SlotLength;               ///< The enumeration value from MISC_SLOT_LENGTH.
  0,    // SlotID;
  {    // SlotCharacteristics1;
    1,  // CharacteristicsUnknown  :1;
    0,  // Provides50Volts         :1;
    0,  // Provides33Volts         :1;
    0,  // SharedSlot              :1;
    0,  // PcCard16Supported       :1;
    0,  // CardBusSupported        :1;
    0,  // ZoomVideoSupported      :1;
    0,  // ModemRingResumeSupported:1;
  },
  {     // SlotCharacteristics2;
    0,  // PmeSignalSupported      :1;
    0,  // HotPlugDevicesSupported :1;
    0,  // SmbusSignalSupported    :1;
    0,  // Reserved                :5;  ///< Set to 0.
  },
  0,    // SegmentGroupNum;
  0,    // BusNum;
  0,    // DevFuncNum;
};
CHAR8  *mSysSlotInfoType9Strings[] = {
  "SD Card",
  NULL
};

/***********************************************************************
	SMBIOS data definition  TYPE16  Physical Memory ArrayInformation
************************************************************************/
SMBIOS_TABLE_TYPE16 mPhyMemArrayInfoType16 = {
  { EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, sizeof (SMBIOS_TABLE_TYPE16), 0 },
  MemoryArrayLocationSystemBoard, // Location;                       ///< The enumeration value from MEMORY_ARRAY_LOCATION.
  MemoryArrayUseSystemMemory,     // Use;                            ///< The enumeration value from MEMORY_ARRAY_USE.
  MemoryErrorCorrectionUnknown,   // MemoryErrorCorrection;          ///< The enumeration value from MEMORY_ERROR_CORRECTION.
  0x40000000,                     // MaximumCapacity;
  0xFFFE,                         // MemoryErrorInformationHandle;
  1,                              // NumberOfMemoryDevices;
  0x40000000ULL,                  // ExtendedMaximumCapacity;
};
CHAR8 *mPhyMemArrayInfoType16Strings[] = {
  NULL
};

/***********************************************************************
	SMBIOS data definition  TYPE17  Memory Device Information
************************************************************************/
SMBIOS_TABLE_TYPE17 mMemDevInfoType17 = {
  { EFI_SMBIOS_TYPE_MEMORY_DEVICE, sizeof (SMBIOS_TABLE_TYPE17), 0 },
  0,          // MemoryArrayHandle; // Should match SMBIOS_TABLE_TYPE16.Handle, initialized at runtime, refer to PhyMemArrayInfoUpdateSmbiosType16()
  0xFFFE,     // MemoryErrorInformationHandle;
  0xFFFF,     // TotalWidth;
  0xFFFF,     // DataWidth;
  0x0400,     // Size; // When bit 15 is 0: Size in MB
              // When bit 15 is 1: Size in KB, and continues in ExtendedSize
  MemoryFormFactorUnknown, // FormFactor;                     ///< The enumeration value from MEMORY_FORM_FACTOR.
  0xff,       // DeviceSet;
  1,          // DeviceLocator String
  2,          // BankLocator String
  MemoryTypeDram,         // MemoryType;                     ///< The enumeration value from MEMORY_DEVICE_TYPE.
  {           // TypeDetail;
    0,  // Reserved        :1;
    0,  // Other           :1;
    1,  // Unknown         :1;
    0,  // FastPaged       :1;
    0,  // StaticColumn    :1;
    0,  // PseudoStatic    :1;
    0,  // Rambus          :1;
    0,  // Synchronous     :1;
    0,  // Cmos            :1;
    0,  // Edo             :1;
    0,  // WindowDram      :1;
    0,  // CacheDram       :1;
    0,  // Nonvolatile     :1;
    0,  // Registered      :1;
    0,  // Unbuffered      :1;
    0,  // Reserved1       :1;
  },
  0,          // Speed;
  3,          // Manufacturer String
  0,          // SerialNumber String
  0,          // AssetTag String
  0,          // PartNumber String
  0,          // Attributes;
  0,          // ExtendedSize;
  0,          // ConfiguredMemoryClockSpeed;
};
CHAR8 *mMemDevInfoType17Strings[] = {
  "OS Virtual Memory",
  "malloc",
  "OSV",
  NULL
};

/***********************************************************************
	SMBIOS data definition  TYPE19  Memory Array Mapped Address Information
************************************************************************/
SMBIOS_TABLE_TYPE19 mMemArrMapInfoType19 = {
  { EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, sizeof (SMBIOS_TABLE_TYPE19), 0 },
  0x00000000, // StartingAddress;
  0x00000000, // EndingAddress;
  0,          // MemoryArrayHandle;
  1,          // PartitionWidth;
  0,          // ExtendedStartingAddress;
  0,          // ExtendedEndingAddress;
};
CHAR8 *mMemArrMapInfoType19Strings[] = {
  NULL
};

/***********************************************************************
	SMBIOS data definition  TYPE32  Boot Information
************************************************************************/
SMBIOS_TABLE_TYPE32 mBootInfoType32 = {
  { EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, sizeof (SMBIOS_TABLE_TYPE32), 0 },
  { 0, 0, 0, 0, 0, 0 },         // Reserved[6];
  BootInformationStatusNoError  // BootStatus
};

CHAR8 *mBootInfoType32Strings[] = {
  NULL
};


/**

  Create SMBIOS record.

  Converts a fixed SMBIOS structure and an array of pointers to strings into
  an SMBIOS record where the strings are cat'ed on the end of the fixed record
  and terminated via a double NULL and add to SMBIOS table.

  SMBIOS_TABLE_TYPE32 gSmbiosType12 = {
    { EFI_SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS, sizeof (SMBIOS_TABLE_TYPE12), 0 },
    1 // StringCount
  };

  CHAR8 *gSmbiosType12Strings[] = {
    "Not Found",
    NULL
  };

  ...

  LogSmbiosData (
    (EFI_SMBIOS_TABLE_HEADER*)&gSmbiosType12,
    gSmbiosType12Strings
    );

  @param  Template    Fixed SMBIOS structure, required.
  @param  StringPack  Array of strings to convert to an SMBIOS string pack.
                      NULL is OK.
  @param  DataSmbiosHande  The new SMBIOS record handle .
                      NULL is OK.
**/

EFI_STATUS
EFIAPI
LogSmbiosData (
  IN  EFI_SMBIOS_TABLE_HEADER *Template,
  IN  CHAR8                   **StringPack,
  OUT EFI_SMBIOS_HANDLE       *DataSmbiosHande
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_PROTOCOL       *Smbios;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER   *Record;
  UINTN                     Index;
  UINTN                     StringSize;
  UINTN                     Size;
  CHAR8                     *Str;

  //
  // Locate Smbios protocol.
  //
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Calculate the size of the fixed record and optional string pack

  Size = Template->Length;
  if (StringPack == NULL) {
    // At least a double null is required
    Size += 2;
  } else {
    for (Index = 0; StringPack[Index] != NULL; Index++) {
      StringSize = AsciiStrSize (StringPack[Index]);
      Size += StringSize;
    }
  if (StringPack[0] == NULL) {
    // At least a double null is required
    Size += 1;
    }

    // Don't forget the terminating double null
    Size += 1;
  }

  // Copy over Template
  Record = (EFI_SMBIOS_TABLE_HEADER *)AllocateZeroPool (Size);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (Record, Template, Template->Length);

  // Append string pack
  Str = ((CHAR8 *)Record) + Record->Length;

  for (Index = 0; StringPack[Index] != NULL; Index++) {
    StringSize = AsciiStrSize (StringPack[Index]);
    CopyMem (Str, StringPack[Index], StringSize);
    Str += StringSize;
  }

  *Str = 0;
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (
                     Smbios,
                     gImageHandle,
                     &SmbiosHandle,
                     Record
                     );

  if ((Status == EFI_SUCCESS) && (DataSmbiosHande != NULL)) {
      *DataSmbiosHande = SmbiosHandle;
  }

  ASSERT_EFI_ERROR (Status);
  FreePool (Record);
  return Status;
}

/***********************************************************************
	SMBIOS data update  TYPE0  BIOS Information
************************************************************************/
VOID
BIOSInfoUpdateSmbiosType0 (
  VOID
  )
{
  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mBIOSInfoType0, mBIOSInfoType0Strings, NULL);
}

/***********************************************************************
	SMBIOS data update  TYPE1  System Information
************************************************************************/
VOID
I64ToHexString(
    IN OUT CHAR8* TargetStringSz,
    IN UINT32 TargetStringSize,
    IN UINT64 Value
    )
{
    static CHAR8 ItoH[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
    UINT8 StringInx;
    INT8 NibbleInx;

    ZeroMem((void*)TargetStringSz, TargetStringSize);

    //
    // Convert each nibble to hex string, starting from
    // the highest non-zero nibble.
    //
    StringInx = 0;
    for (NibbleInx = sizeof(UINT64) * 2; NibbleInx > 0; --NibbleInx) {
        UINT64 NibbleMask = (((UINT64)0xF) << ((NibbleInx - 1) * 4));
        UINT8 Nibble = (UINT8)((Value & NibbleMask) >> ((NibbleInx - 1) * 4));

        ASSERT(Nibble <= 0xF);

        if (StringInx < (TargetStringSize-1)) {
            TargetStringSz[StringInx++] = ItoH[Nibble];
        } else {
            break;
        }
    }
}

VOID
SysInfoUpdateSmbiosType1 (
  VOID
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT64 BoardSerial = 0xdeadface;
  static CHAR8 BoardSerialString[sizeof(BoardSerial) * 2 + 1];
  int k=0;

  Status = mFwProtocol->GetSerial(&BoardSerial);
  if (EFI_ERROR(Status)) {
      //
      // On error, just log and leave the template string
      //
      DEBUG ((EFI_D_ERROR, "Failed to get board serial number, status 0x%08X\n", Status));
  } else {
      //
      // Convert to HEX string
      //
      I64ToHexString(&BoardSerialString[0], sizeof(BoardSerialString), BoardSerial);

      //
      // Update the Smbios Type1 information with the board serial string
      //
      mSysInfoType1Strings[mSysInfoType1.SerialNumber - 1] = &BoardSerialString[0];

      DEBUG ((EFI_D_ERROR, "Board Serial Number: %lx\n", BoardSerial));

      // construct string for making UUID for Rpi3 board from : fixed prefix + board serial number printed in hex
      mSysInfoType1.Uuid.Data1= *(UINT32 *)"RPi3";
      mSysInfoType1.Uuid.Data2=0x0;
      mSysInfoType1.Uuid.Data3=0x0;
      for(k=7; k>=0; k--){

         mSysInfoType1.Uuid.Data4[7-k]=(UINT8)(BoardSerial>>(k*8));
      };

      //
      // example of GUID created 52706932-0000-0000-0000-0000096554c1
      //
      // option: compute SHA1 hash of string from GUID constructed and use it as Uuid
      //
  }

  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mSysInfoType1, mSysInfoType1Strings, NULL);
}

/***********************************************************************
	SMBIOS data update  TYPE2  Board Information
************************************************************************/
VOID
BoardInfoUpdateSmbiosType2 (
  VOID
  )
{
  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mBoardInfoType2, mBoardInfoType2Strings, NULL);
}

/***********************************************************************
	SMBIOS data update  TYPE3  Enclosure Information
************************************************************************/
VOID
EnclosureInfoUpdateSmbiosType3 (
  VOID
  )
{
  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mEnclosureInfoType3, mEnclosureInfoType3Strings, NULL);
}

/***********************************************************************
	SMBIOS data update  TYPE4  Processor Information
************************************************************************/
VOID
ProcessorInfoUpdateSmbiosType4 (
  IN UINTN MaxCpus
  )
{
  EFI_STATUS Status;
  UINT32 Rate;

  mProcessorInfoType4.CoreCount        = (UINT8) MaxCpus;
  mProcessorInfoType4.EnabledCoreCount = (UINT8) MaxCpus;
  mProcessorInfoType4.ThreadCount      = (UINT8) MaxCpus;

  Status = mFwProtocol->GetClockRate(RPI_FW_CLOCK_RATE_ARM, &Rate);
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Couldn't get the current CPU speed: %r\n", Status));
  } else {
    mProcessorInfoType4.CurrentSpeed = Rate / 1000000;
    DEBUG ((DEBUG_INFO, "Current CPU speed: %uHz\n", Rate));
  }

  Status = mFwProtocol->GetMaxClockRate(RPI_FW_CLOCK_RATE_ARM, &Rate);
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Couldn't get the max CPU speed: %r\n", Status));
  } else {
    mProcessorInfoType4.MaxSpeed = Rate / 1000000;
    DEBUG ((DEBUG_INFO, "Max CPU speed: %uHz\n", Rate));
  }

  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mProcessorInfoType4, mProcessorInfoType4Strings, NULL);
}

/***********************************************************************
	SMBIOS data update  TYPE7  Cache Information
************************************************************************/
VOID
CacheInfoUpdateSmbiosType7 (
  VOID
  )
{
  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mCacheInfoType7, mCacheInfoType7Strings, NULL);
}

/***********************************************************************
	SMBIOS data update  TYPE9  System Slot Information
************************************************************************/
VOID
SysSlotInfoUpdateSmbiosType9 (
  VOID
  )
{
  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mSysSlotInfoType9, mSysSlotInfoType9Strings, NULL);
}

/***********************************************************************
	SMBIOS data update  TYPE16  Physical Memory Array Information
************************************************************************/
VOID
PhyMemArrayInfoUpdateSmbiosType16 (
  VOID
  )
{
  EFI_SMBIOS_HANDLE MemArraySmbiosHande;

  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mPhyMemArrayInfoType16, mPhyMemArrayInfoType16Strings, &MemArraySmbiosHande);

  //
  // Update the memory device information
  //
  mMemDevInfoType17.MemoryArrayHandle = MemArraySmbiosHande;
}

/***********************************************************************
	SMBIOS data update  TYPE17  Memory Device Information
************************************************************************/
VOID
MemDevInfoUpdateSmbiosType17 (
  VOID
  )
{
  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mMemDevInfoType17, mMemDevInfoType17Strings, NULL);
}

/***********************************************************************
	SMBIOS data update  TYPE19  Memory Array Map Information
************************************************************************/
VOID
MemArrMapInfoUpdateSmbiosType19 (
  VOID
  )
{
  EFI_STATUS Status;
  UINT32 Base;
  UINT32 Size;

  Status = mFwProtocol->GetArmMem(&Base, &Size);
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "Couldn't get the ARM memory size: %r\n", Status));
  } else {
    mMemArrMapInfoType19.StartingAddress = Base / 1024;
    mMemArrMapInfoType19.EndingAddress = (Base + Size - 1) / 1024;
  }

  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mMemArrMapInfoType19, mMemArrMapInfoType19Strings, NULL);
}


/***********************************************************************
	SMBIOS data update  TYPE32  Boot Information
************************************************************************/
VOID
BootInfoUpdateSmbiosType32 (
  VOID
  )
{
  LogSmbiosData ((EFI_SMBIOS_TABLE_HEADER *)&mBootInfoType32, mBootInfoType32Strings, NULL);
}

/***********************************************************************
	Driver Entry
************************************************************************/
EFI_STATUS
EFIAPI
PlatformSmbiosDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (&gRaspberryPiFirmwareProtocolGuid, NULL,
                  (VOID **)&mFwProtocol);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BIOSInfoUpdateSmbiosType0();

  SysInfoUpdateSmbiosType1();

  BoardInfoUpdateSmbiosType2();

  EnclosureInfoUpdateSmbiosType3();

  ProcessorInfoUpdateSmbiosType4 (4);	//One example for creating and updating

  CacheInfoUpdateSmbiosType7();

  SysSlotInfoUpdateSmbiosType9();

  PhyMemArrayInfoUpdateSmbiosType16();

  MemDevInfoUpdateSmbiosType17();

  MemArrMapInfoUpdateSmbiosType19();

  BootInfoUpdateSmbiosType32();

  return EFI_SUCCESS;
}
