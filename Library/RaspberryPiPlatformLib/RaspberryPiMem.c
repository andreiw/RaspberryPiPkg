/** @file
*
*  Copyright (c) 2014, Linaro Limited. All rights reserved.
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

#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

extern UINT64 mSystemMemoryEnd;

STATIC ARM_MEMORY_REGION_DESCRIPTOR RaspberryPiMemoryRegionDescriptor[] = {
  {
    FixedPcdGet64 (PcdFdBaseAddress),
    FixedPcdGet64 (PcdFdBaseAddress),
    FixedPcdGet32 (PcdFdSize),
    ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
  },
  {
    FixedPcdGet64 (PcdSystemMemoryBase),
    FixedPcdGet64 (PcdSystemMemoryBase),
    0,
    ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
  },
  {
    0x3f000000,
    0x3f000000,
    0x02000000,
    ARM_MEMORY_REGION_ATTRIBUTE_DEVICE
  },
  {
  }
};

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU
  on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR
                                    describing a Physical-to-Virtual Memory
                                    mapping. This array must be ended by a
                                    zero-filled entry

**/
VOID
ArmPlatformGetVirtualMemoryMap (
  IN ARM_MEMORY_REGION_DESCRIPTOR** VirtualMemoryMap
  )
{
  RaspberryPiMemoryRegionDescriptor[1].Length = mSystemMemoryEnd + 1 -
                                                FixedPcdGet64 (PcdSystemMemoryBase);

  DEBUG ((EFI_D_INFO, "%a: Dumping System DRAM Memory Map:\n"
      "\tPhysicalBase: 0x%lX\n"
      "\tVirtualBase: 0x%lX\n"
      "\tLength: 0x%lX\n",
      __FUNCTION__,
      RaspberryPiMemoryRegionDescriptor[1].PhysicalBase,
      RaspberryPiMemoryRegionDescriptor[1].VirtualBase,
      RaspberryPiMemoryRegionDescriptor[1].Length));

  *VirtualMemoryMap = RaspberryPiMemoryRegionDescriptor;
}
