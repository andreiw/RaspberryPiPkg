# @file
#
#  Copyright (c), 2018, Andrei Warkentin <andrey.warkentin@gmail.com>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

ifeq ($(ACPI_PINFUNCTION), 1)
	IASL_FLAGS = -DACPI_PINFUNCTION
else
	IASL_FLAGS =
endif

all: acpi_offsets
genc: acpi_offsets
genmake: acpi_offsets
modules: acpi_offsets
libraries: acpi_offsets
fds: acpi_offsets
clean: clean_acpi_offsets
cleanall: clean_acpi_offsets
cleanlib: clean_acpi_offsets
run: 

clean_acpi_offsets:
	rm $(WORKSPACE)/RaspberryPiPkg/Include/DSDT.offset.h

acpi_offsets:
	$(IASL_PREFIX)iasl $(IASL_FLAGS) -so $(WORKSPACE)/RaspberryPiPkg/AcpiTables/DSDT.asl
	rm -f $(WORKSPACE)/RaspberryPiPkg/AcpiTables/*.aml        
	mv $(WORKSPACE)/RaspberryPiPkg/AcpiTables/DSDT.offset.h $(WORKSPACE)/RaspberryPiPkg/Include

