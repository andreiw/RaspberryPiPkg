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
#include <IndustryStandard/Bcm2836.h>
#include <Library/PcdLib.h>

extern UINT64 mSystemMemoryEnd;
extern UINT64 mGPUMemoryBase;
extern UINT64 mGPUMemoryLength;

STATIC ARM_MEMORY_REGION_DESCRIPTOR RaspberryPiMemoryRegionDescriptor[] = {
  {
    /* Firmware Volume. */
    FixedPcdGet64 (PcdFdBaseAddress),
    FixedPcdGet64 (PcdFdBaseAddress),
    FixedPcdGet32 (PcdFdSize),
    ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
  },
  {
    /* ATF reserved RAM. */
    FixedPcdGet64 (PcdFdBaseAddress) + FixedPcdGet32 (PcdFdSize),
    FixedPcdGet64 (PcdFdBaseAddress) + FixedPcdGet32 (PcdFdSize),
    FixedPcdGet64 (PcdSystemMemoryBase) -
    (FixedPcdGet64 (PcdFdBaseAddress) + FixedPcdGet32 (PcdFdSize)),
    ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
  },
  {
    /* System RAM. */
    FixedPcdGet64 (PcdSystemMemoryBase),
    FixedPcdGet64 (PcdSystemMemoryBase),
    0,
    ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
  },
  {
    /* Reserved GPU RAM. */
    0,
    0,
    0,
    ARM_MEMORY_REGION_ATTRIBUTE_DEVICE
  },
  {
    /* SOC registers. */
    BCM2836_SOC_REGISTERS,
    BCM2836_SOC_REGISTERS,
    BCM2836_SOC_REGISTER_LENGTH,
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
  RaspberryPiMemoryRegionDescriptor[2].Length = mSystemMemoryEnd + 1 -
    FixedPcdGet64 (PcdSystemMemoryBase);

  RaspberryPiMemoryRegionDescriptor[3].PhysicalBase =
    RaspberryPiMemoryRegionDescriptor[2].PhysicalBase +
    RaspberryPiMemoryRegionDescriptor[2].Length;

  RaspberryPiMemoryRegionDescriptor[3].VirtualBase =
    RaspberryPiMemoryRegionDescriptor[3].PhysicalBase;

  RaspberryPiMemoryRegionDescriptor[3].Length =
    RaspberryPiMemoryRegionDescriptor[4].PhysicalBase -
    RaspberryPiMemoryRegionDescriptor[3].PhysicalBase;

  DEBUG ((EFI_D_INFO, "FD:\n"
          "\tPhysicalBase: 0x%lX\n"
          "\tVirtualBase: 0x%lX\n"
          "\tLength: 0x%lX\n",
          RaspberryPiMemoryRegionDescriptor[0].PhysicalBase,
          RaspberryPiMemoryRegionDescriptor[0].VirtualBase,
          RaspberryPiMemoryRegionDescriptor[0].Length));

  DEBUG ((EFI_D_INFO, "ATF RAM:\n"
          "\tPhysicalBase: 0x%lX\n"
          "\tVirtualBase: 0x%lX\n"
          "\tLength: 0x%lX\n",
          RaspberryPiMemoryRegionDescriptor[1].PhysicalBase,
          RaspberryPiMemoryRegionDescriptor[1].VirtualBase,
          RaspberryPiMemoryRegionDescriptor[1].Length));

  DEBUG ((EFI_D_INFO, "System RAM:\n"
          "\tPhysicalBase: 0x%lX\n"
          "\tVirtualBase: 0x%lX\n"
          "\tLength: 0x%lX\n",
          RaspberryPiMemoryRegionDescriptor[2].PhysicalBase,
          RaspberryPiMemoryRegionDescriptor[2].VirtualBase,
          RaspberryPiMemoryRegionDescriptor[2].Length));

  DEBUG ((EFI_D_INFO, "GPU Reserved:\n"
          "\tPhysicalBase: 0x%lX\n"
          "\tVirtualBase: 0x%lX\n"
          "\tLength: 0x%lX\n",
          RaspberryPiMemoryRegionDescriptor[3].PhysicalBase,
          RaspberryPiMemoryRegionDescriptor[3].VirtualBase,
          RaspberryPiMemoryRegionDescriptor[3].Length));

  DEBUG ((EFI_D_INFO, "SoC reserved:\n"
          "\tPhysicalBase: 0x%lX\n"
          "\tVirtualBase: 0x%lX\n"
          "\tLength: 0x%lX\n",
          RaspberryPiMemoryRegionDescriptor[4].PhysicalBase,
          RaspberryPiMemoryRegionDescriptor[4].VirtualBase,
          RaspberryPiMemoryRegionDescriptor[4].Length));

  *VirtualMemoryMap = RaspberryPiMemoryRegionDescriptor;
}
