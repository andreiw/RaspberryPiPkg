/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
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

#include <PiPei.h>

#include <Library/ArmMmuLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

extern UINT64 mSystemMemoryEnd;

VOID
BuildMemoryTypeInformationHob (
  VOID
  );

STATIC
VOID
InitMmu (
  IN ARM_MEMORY_REGION_DESCRIPTOR  *MemoryTable
  )
{

  VOID                          *TranslationTableBase;
  UINTN                         TranslationTableSize;
  RETURN_STATUS                 Status;

  //Note: Because we called PeiServicesInstallPeiMemory() before to call InitMmu() the MMU Page Table resides in
  //      DRAM (even at the top of DRAM as it is the first permanent memory allocation)
  Status = ArmConfigureMmu (MemoryTable, &TranslationTableBase, &TranslationTableSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Error: Failed to enable MMU\n"));
  }
}

STATIC
VOID
AddAndReserved(ARM_MEMORY_REGION_DESCRIPTOR *Desc)
{
  BuildResourceDescriptorHob (
                              EFI_RESOURCE_SYSTEM_MEMORY,
                              EFI_RESOURCE_ATTRIBUTE_PRESENT |
                              EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
                              EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
                              EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
                              EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
                              EFI_RESOURCE_ATTRIBUTE_TESTED,
                              Desc->PhysicalBase,
                              Desc->Length
                              );

  BuildMemoryAllocationHob (
                            Desc->PhysicalBase,
                            Desc->Length,
                            EfiReservedMemoryType
                            );
}

STATIC
VOID
AddAndMmio(ARM_MEMORY_REGION_DESCRIPTOR *Desc)
{
  BuildResourceDescriptorHob (
                              EFI_RESOURCE_SYSTEM_MEMORY,
                              (EFI_RESOURCE_ATTRIBUTE_PRESENT    |
                               EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
                               EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
                               EFI_RESOURCE_ATTRIBUTE_TESTED),
                              Desc->PhysicalBase,
                              Desc->Length
                              );

  BuildMemoryAllocationHob (
                            Desc->PhysicalBase,
                            Desc->Length,
                            EfiMemoryMappedIO
                            );
}

/*++

Routine Description:



Arguments:

  FileHandle  - Handle of the file being invoked.
  PeiServices - Describes the list of possible PEI Services.

Returns:

  Status -  EFI_SUCCESS if the boot mode could be set

--*/
EFI_STATUS
EFIAPI
MemoryPeim (
  IN EFI_PHYSICAL_ADDRESS               UefiMemoryBase,
  IN UINT64                             UefiMemorySize
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR *MemoryTable;

  // Get Virtual Memory Map from the Platform Library
  ArmPlatformGetVirtualMemoryMap (&MemoryTable);

  // Ensure PcdSystemMemorySize has been set
  ASSERT (PcdGet64 (PcdSystemMemorySize) != 0);

  // Reserved FD + Trusted Firmware region
  AddAndReserved(&MemoryTable[0]);
  AddAndReserved(&MemoryTable[1]);

  // Usable memory.
  BuildResourceDescriptorHob (
                              EFI_RESOURCE_SYSTEM_MEMORY,
                              EFI_RESOURCE_ATTRIBUTE_PRESENT |
                              EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
                              EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
                              EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
                              EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
                              EFI_RESOURCE_ATTRIBUTE_TESTED,
                              MemoryTable[2].PhysicalBase,
                              MemoryTable[2].Length
                              );

  AddAndReserved(&MemoryTable[3]);
  AddAndMmio(&MemoryTable[4]);

  // Build Memory Allocation Hob
  InitMmu (MemoryTable);

  if (FeaturePcdGet (PcdPrePiProduceMemoryTypeInformationHob)) {
    // Optional feature that helps prevent EFI memory map fragmentation.
    BuildMemoryTypeInformationHob ();
  }

  return EFI_SUCCESS;
}
