64-bit Tiano Core UEFI for the Raspberry Pi 3
=============================================

Last updated Jan 16th, 2018.

This is a port of 64-bit Tiano Core UEFI firmware for the Pi 3/3B+ platforms,
based on [Ard Bisheuvel's 64-bit](http://www.workofard.com/2017/02/uefi-on-the-pi/)
and [Microsoft's 32-bit](https://github.com/ms-iot/RPi-UEFI/tree/ms-iot/Pi3BoardPkg) implementations.

Initially, this was supposed to be an easy walk in the park, where
the Microsoft drivers just sorta slid into Ard's UEFI implementation,
and I would call it a day. Instead, it turned out to be a severely more
frustrating experience :-).

# Purpose

This is meant as a generally useful 64-bit ATF + UEFI implementation for the Pi 3/3B+,
good enough for most kinds of UEFI development and good enough for running real operating
systems. It has been validated to install and boot Linux (SUSE, Ubuntu), NetBSD and
FreeBSD, and there is experimental (64-bit) Windows on Arm support as well.
It wound up being the early development platform for NetBSD's 64-bit Arm UEFI
bootloader.

It's mostly EBBR compliant, favoring user experience over pedantic compliance
where those two are in conflict. With enough HypDxe grease it may even, some day,
pass for an SBSA + SBBR system ;-).

# Latest Status

* 2019 Jam 16th: boot options fixes, _CCA, supported firmware for Windows MCCI driver.
* 2019 Jan 14th: boot option cleanup, EBC, release, SPCR fix (Windows EMS support)
* 2018 Nov 17th: Display, USB, GraphicsConsole, VirtualRealTimeClockLib improvements, edk2 rebase.
* 2018 Oct 1st: Rhxp and PEP devices in ACPI, (untested) JTAG support via debug configuration menu.
* 2018 Sep 29th: MsftFunctionConfig ACPI descriptors.
* 2018 Sep 28th: SMBIOS nits, clear screen before boot, SPCR InterruptType, DWC_OTG range and extra _CID.
* 2018 Sep 18th: PXE boot order fix, serial prompting, improved variable dumping
* 2018 Sep 14th: fix FADT minor version (PSCI detection) and SMBIOS regression (seen in Windows).
* 2018 Jul 8th: switch to private MmcDxe, improve multiblock write robustness, power-off/halt in ATF
* 2018 Jun 26th: SdHostDxe error handling, early eMMC support for both SdHost and Arasan
* 2018 Jun 22nd: Arasan and SdHost multiblock, Arasan/SdHost/MMC tweaks/stability, uSD default routing is SdHost
* 2018 Jun 17th: SdHostDxe boot order fix, MMC tweaks menu, GCC5_DEBUG fix.
* 2018 Jun 16th: SdHostDxe support, HypDxe on by default.
* 2018 Jun 13th: Fix GCC5 compilation error for HypDxe.
* 2018 Jun 12th: Fix SMBIOS Type 0 BIOS date.
* 2018 Jun 7th:  HypDxe can redirect WoA kernel messages to UART.
* 2018 May 27th: allow 640 x 480 and lower resolutions.
* 2018 May 24th: fix WoA regression reported with build 17134.
* 2018 May 22nd: can boot 64-bit Windows on Arm without WinDbg (and without hacky ATF).
* 2018 May 19th: Ax88772b USB NIC (not onboard SMSC95xx) PXE boot.
* 2018 May 18th: allow changing Arm frequency settings (do nothing, force 600MHz or max)
* 2018 May 14th: rebase to current edk2 upstream (still under verification, no bin release).
* 2018 May 13th: set maximum Arm frequency, better info/smbios, DisplayDxe fix.
* 2018 May 12th: updated May 9th build VC firmware to support RPi3.
* 2018 May 9th:  pseudo-NVRAM, persisted RTC, Arasan controller ACPI description, USB fix.
* 2018 Apr 24th: SMP support in WoA.
* 2018 Apr 22nd: switched to MS-IoT ACPI, can boot 64-bit WoA WinPE with WinDbg.
* 2018 Apr 21st: improved booting experience, removed BGRT, WoA docs.
* 2018 Apr 5th:  improved ACPI (FADT, GTDT, SPCR, BGRT, MADT), UEFI implementation comparison.
* 2018 Mar 31st: updated supported keyboard info.
* 2018 Mar 1st:  updated ATF to fix Ubuntu poweroff crash and add directions.
* 2018 Feb 27th: updated ATF to fix overheat on SYSTEM_OFF.
* 2018 Feb 26th: improved USB driver and HS support.
* 2018 Feb 22nd: improved USB support for keyboards.
* 2018 Jan 13th: updated build instructions, information on keyboards supported, added prebuilts.
* 2017 Dec 26th: USB hotplug and keyboard support.
* 2017 Dec 15th: Initial release.

# Features

Here is a comparison table between different available EFI firmware implementations
for the RPi3.

| Feature | This Implementation | Ard's  | Microsoft's | U-Boot | Minoca |
| ------- | ------------------- | ------ | ----------- | ------ | ------ |
| **Bitness** | 64-bit | 64-bit | 32-bit | Either | 32-bit |
| **PSCI CPU_ON**  | Yes | No | No | No | No |
| **PSCI SYSTEM_RESET** | Yes | Yes | No | No | No |
| **PSCI SYSTEM_OFF** | Yes | No | No | No | No |
| **DT** | Yes | Yes | No | Yes | No |
| **Pass-through DT** | Yes | No | N/A | Yes | No |
| **NVRAM** | Limited | No | No | No | No |
| **RTC** | Limited | No | No | No | No |
| **ACPI** | Yes | No | Yes | No | Yes |
| **Serial** | Yes | Yes | Yes | Yes | Yes |
| **HDMI GOP** | Yes | No | No | Yes | No |
| **SMBIOS** | Yes | No | Yes | No | Yes |
| **uSD** | Yes | No | Yes | Yes | Yes |
| **uSD SdHost and Arasan** | Yes | No | Yes | ? | No |
| **USB1** | Limited | No | No | Yes | No |
| **USB2/3** | Yes | No | No | Yes | No | No |
| **USB Mass Storage** | Yes  | No | No | Yes | No |
| **USB Keyboard** | Yes | No | No | Yes | No |
| **USB Ax88772b PXE/Network** | Yes | No | No | Yes | No |
| **USB SMSC95xx PXE/Network** | No | No | No | Yes | No |
| **Tiano** | Yes | Yes | Yes | No | No |
| **AArch32 Windows IoT** | No | No | Yes | No | No |
| **AArch64 Windows on Arm** | Limited | No | No | No | No |
| **AArch64 Linux** | Yes | Limited | No | Yes | No |
| **AArch32 Linux** | No | No | No | Yes | No |
| **AArch64 FreeBSD** | Yes | No | No | Yes | No |
| **AArch32 Minoca** | No | No | No | No | Yes |

# Building

**Note: If you want to use the [pre-built UEFI images](Binary/prebuilt), you can skip this section.**

1. Clone `https://github.com/tianocore/edk2.git`

This is the last known good edk2 commit:
```
commit 66127011a544b90e800eb3619e84c2f94a354903
Author: Ard Biesheuvel <ard.biesheuvel@linaro.org>
Date:   Wed Nov 14 11:27:24 2018 -0800

    ArmPkg/ArmGicDxe ARM: fix encoding for GICv3 interrupt acknowledge
```

You should rewind your edk2 tree to this commit. Here be dragons!

2. Clone this repo.

3. Apply the [various patches against the edk2 tree](edk2Patches). Yes, it sucks to have to do this, but this is a clearer way forward than forking every single Tiano driver that has a bug in it, or worse - carrying around an entire private fork of edk2. You're welcome to upstream these patches!

To avoid issues, apply using `--ignore-whitespace`. E.g.:
```
$ git am --ignore-whitespace ../RaspberryPiPkg/edk2Patches/*.patch
```

4. Use one of the [provided](Scripts/build49) [templates](Scripts/build5) for your build script. If you use a different GCC version, change accordingly,
and adjust the compiler prefix to match your system - i.e. set `GCC49_AARCH64_PREFIX` if you're passing `-t GCC49` to `build`.

If you want to build your own ATF, instead of using the checked-in binaries, follow
the additional directions under [`Binary/atf/readme.md`](Binary/atf/readme.md).

# Using

## Basic

UEFI boot media can be a uSD card or USB mass storage, if you've enabled USB booting previously in the OTP (i.e. via `program_usb_boot_mode=1`).

UEFI boot media must be MBR partitioned and FAT32 formatted.

As a starting point, take [one of the latest RELEASE prebuilt image directories](Binary/prebuilt) and copy contents to empty boot media. If you've built your own UEFI from source (e.g. `$WORKSPACE/Build/RaspberryPiPkg-AARCH64/RELEASE_GCC5/FV/RPI_EFI.fd`) you can simply now copy over and overwrite `RPI_EFI.fd`.

**Note: You *may not* have a kernel.img (or kernelX.img, where X is a digit) in the root
catalogue of the boot media. It will *not* boot.**

The most basic `config.txt` contents are:
```
arm_control=0x200
enable_uart=1
armstub=RPI_EFI.fd
disable_commandline_tags=1
```

This will boot UEFI and expose an RPi3 device tree that is compatible with openSUSE Leap 42.2/42.3, although it was found to work with Ubuntu 18.04 (Bionic Beaver) as well.

Of course use the debug variant (e.g. `$WORKSPACE/Build/RaspberryPiPkg-AARCH64/DEBUG_GCC5/FV/RPI_EFI.fd`) if necessary, but it will boot a lot slower due to the verbose spew.

HDMI and the mini-UART serial port can be used for output devices. Output is mirrored.
USB keyboards and the mini-UART serial port can be used as input.

USB keyboard support has been validated with a few keyboards:
- Logitech K750 (wireless)
- Dell SK-8125 keyboard (with built-in hub)
- Microsoft Natural Ergonomic Keyboard 4000
- An Apple keyboard (chicklet, USB2 hub)

The first time you boot, you will be looking at the UEFI Shell. 'exit'
and modify the boot order. The boot order will persist across reboots.
The boot manager will only list devices available to boot from
(older versions had USB Port 0, USB Port 1, etc).

ESC enters setup. F1 always boots the UEFI Shell.

![FrontPage](readme1.png)

Note: you cannot boot 32-bit OSes like Raspbian with this firmware. Aw, shucks, right?

## Custom Device Tree

Most likely, if you boot an OS other than openSUSE Leap 42.3, you will need to pass your own
distro- and kernel- specific device tree. This will need to be extracted from the
distributed media or from a running system (e.g that was booted via U-Boot).

This involves a few changes to the above `config.txt`:
```
...
disable_commandline_tags=2
device_tree_address=0x8000
device_tree_end=0x10000
device_tree=my_fdt.dtb
```

Note: the address range **must** be [0x8000:0x10000). `dtoverlay` and `dtparam` parameters are also supported.

## Custom `bootargs`

This firmware will honor the command line passed by the GPU via `cmdline.txt`.

Note, that the ultimate contents of `/chosen/bootargs` are a combination of several pieces:
- Original `/chosen/bootargs` if using the internal DTB. Seems to be completely discarded by GPU when booting with a custom device tree.
- GPU-passed hardware configuration. This one is always present.
- Additional boot options passed via `cmdline.txt`.

## openSUSE Leap 42.3

Untested with the Pi 3 B+. You may need to get the latest device tree and [follow the instructions](#custom-device-tree).

Download the Leap 42.3 RPi image first, from `http://download.opensuse.org/ports/aarch64/distribution/leap/42.3/appliances/` (e.g. `openSUSE-Leap42.3-ARM-XFCE-raspberrypi3.aarch64-2017.07.26-Build1.1` was good).

- `dd` image to media.
- If booting UEFI from same media:
  - In the `EFI` partition, delete everything but the `EFI` folder.
  - Follow the [basic steps for booting UEFI](#basic).

Login is `root`/`linux`. There is also a login available on the serial port.

**Note: if your media is USB, after first boot you must follow these steps, or you will have an unbootable system after first reboot:**
- Edit the file `/etc/dracut.conf.d/raspberrypi_modules.conf` to include as its first line:
```add_drivers+=" bcm2835-sdhost bcm2835_dma sdhci_bcm2835 dwc2 usbnet uas usb_storage usbcore usb_common "```
- mkinitrd

You may choose to remove `enable_uart=1` from `config.txt` to get your RPi3 to run
at full speed.

If you wish to use virtualization (e.g. KVM), you must configure UEFI to boot in EL2 mode. In UEFI setup screen:
- go to `Device Manager`
- go to `Raspberry Pi Configuration`
- go to `HypDxe Configuration`
- configure `System Boot Mode` as `Boot in EL2`
- after saving, Pi will reset itself.

## Ubuntu (18.04 Bionic Beaver)

Untested with the Pi 3 B+. You may need to get the latest device tree and [follow the instructions](#custom-device-tree).

- Download `http://ports.ubuntu.com/ubuntu-ports/dists/bionic/main/installer-arm64/current/images/netboot/mini.iso` and write out to a USB stick.
- Boot installer.
- Install to another USB stick (uSD slot is not available).
- Enjoy. uSD slot will be available as mmcblk0.

There is a device tree blob under `http://ports.ubuntu.com/ubuntu-ports/dists/bionic/main/installer-arm64/current/images/device-tree/bcm2837-rpi-3-b.dtb`, which you will [need to use](#custom-device-tree) to if you want Wi-Fi and Bluetooth, but otherwise things seem to work just fine with
the bundled openSUSE Leap 42.2 device tree.

Note: don't use DEBUG builds of ATF (e.g. DEBUG builds of UEFI) with Ubuntu, as the latter
disables the mini-UART port, which the ATF relies on for logging. If you want to use a DEBUG
build of UEFI, you must use a release version of ATF. Follow the directions under [`Binary/atf/readme.md`](Binary/atf/readme.md).

For Wi-Fi and BT there are a few more steps, as certain firmware files appear to be missing from the installation:
- `cd /lib/firmware/brcm/`
- `wget https://raw.githubusercontent.com/RPi-Distro/firmware-nonfree/master/brcm/brcmfmac43430-sdio.txt`
- `wget https://github.com/OpenELEC/misc-firmware/raw/master/firmware/brcm/BCM43430A1.hcd`
- `apt-get install wireless-regdb`

If you wish to use virtualization (e.g. KVM), you must configure UEFI to boot in EL2 mode. In UEFI setup screen:
- go to `Device Manager`
- go to `Raspberry Pi Configuration`
- go to `HypDxe Configuration`
- configure `System Boot Mode` as `Boot in EL2`
- after saving, Pi will reset itself.

## FreeBSD (r326622)

Untested with the Pi 3 B+. You may need to get the latest device tree and [follow the instructions](#custom-device-tree).

- Download
`http://ftp.freebsd.org/pub/FreeBSD/snapshots/ISO-IMAGES/12.0/FreeBSD-12.0-CURRENT-arm64-aarch64-RPI3-20171206-r326622.img.xz`
- Uncompress and `dd` to media.
- If booting UEFI from same media:
  - Delete everything from the `MSDOSBOOT` partition, except:
    - `EFI`
    - `overlays`
    - `bcm2710-rpi-3-b.dtb`
  - Follow the [basic steps for booting UEFI to boot from media](#basic).
- If not booting UEFI from same media:
  - Follow the [basic steps for booting UEFI to boot from media](#basic).
  - Copy `bcm2710-rpi-3-b.dtb` and `overlays` to the UEFI boot media.

Now replace `config.txt` in the UEFI boot media with:
```
arm_control=0x200
armstub=RPI_EFI.fd
enable_uart=1
disable_commandline_tags=2
dtoverlay=mmc
dtparam=audio=on,i2c_arm=on,spi=on
device_tree_address=0x8000
device_tree_end=0x10000
device_tree=bcm2710-rpi-3-b.dtb
```

For a different (newer) release, you will need to look at the original `config.txt`.

This should boot to login prompt on HDMI with USB HID as the input. Login is `root`/`root`.

Note: you must remove `dtoverlay=pi3-disable-bt`, if present, from `config.txt`, as both ATF and UEFI rely on the mini-UART being initialized.

PL011 serial console in FreeBSD is not supported, yet.

## 64-bit Windows on Arm

Builds 17125-17134, 17672 are known to work.

Validated with a Pi3B+ as well.

To try:
- Get a Windows 10 host.
- Download the Windows ADK Insider Preview (matching one of the builds above).
- Install ADK (Deployment Tools and Windows Preinstallation Environment).
- If your Windows 10 host is arm64, you will need to patch the `DandISetEnv.bat`
  script to `SET PROCESSOR_ARCHITECTURE=x86` to get the `Deployment and
  Imaging Tools` environment working to run the next few commands.
- `copype arm64 C:\WinPE_arm64`
- `MakeWinPEMedia /ufd C:\WinPE_arm64 <usb drive letter:>`
- Follow the [basic steps for booting UEFI](#basic).

System should boot to a single `cmd.exe` window.

Note: there are no built-in drivers for anything.

Note: if `HypDxe` is configured to `Boot in EL2`, Windows on Arm will not boot. The remaining HypDxe configuration options are developer-oriented.

https://github.com/andreiw/RaspberryPiPkg/issues/12 for related discussion.

Also see https://www.worproject.ml/ and https://www.worproject.ml/bugtracker (not affiliated with RaspberryPiPkg).

# Bugs in Implemented Functionality

## HDMI

The UEFI HDMI video support relies on the VC (that's the GPU)
firmware to correctly detect and configure the attached screen.
Some screens are slow, and this detection may not occur fast
enough. Finally, you may wish to be able to boot your Pi
headless, yet be able to attach a display to it later for
debugging.

To accomodate these issues, the following extra lines
are recommended for your `config.txt`:
- `hdmi_force_hotplug=1` to allow plugging in video after system is booted.
- `hdmi_group=1` and `hdmi_mode=4` to force a specific mode, both to accomodate late-plugged screens or buggy/slow screens. See [official documentation](https://www.raspberrypi.org/documentation/configuration/config-txt/video.md) to make sense of these parameters (example above sets up 720p 60Hz).

While the VC firmware is reponsible for setting the physical resolution,
the virtual resolution the GPU framebuffer uses may be different and it
scales the video appropriately. By default, the UEFI framebuffer driver
makes available the following virtual resolutions:
- 640x480 (32bpp)
- 800x600 (32bpp)
- 1024x768 (32bpp)
- 1280x720 (32bpp)
- 1920x1080 (32bpp)
- native physical resolution (32bpp)

Note that this lets you do weird stuff, like pretending to have 1080p while
connected to a TV. Sometimes blurry is better than nothing...

Note: the VC framebuffer is a bit weird and will change physical locations
      depending on virtual resolution chosen. Some UEFI applications or
      OS loaders may violate the GOP spec and never refresh the framebuffer
      addressing after setting the mode. You can completely disable
      multiple virtual resolution support:
- go to `Device Manager`
- go to `Raspberry Pi Configuration`
- go to `Display`
- configure `Resolutions` to `Only native resolution`

## NVRAM

The Raspberry Pi has no NVRAM.

NVRAM is emulated, with the non-volatile store backed by the UEFI image itself. This means
that any changes made in UEFI proper will be persisted, but changes made in HLOS will not.
It would be nice to implement ATF-assisted warm reboot, to allow persisting HLOS
NVRAM changes.

## RTC

The Rasberry Pi has no RTC.

`RtcEpochSeconds` NVRAM variable is used to store the boot time
This should allow you to set whatever date/time you
want using the Shell date and time commands. While in UEFI
or HLOS, the time will tick forward. `RtcEpochSeconds`
is not updated on reboots.

## uSD

UEFI supports both the Arasan SDHCI and the Broadcom SDHost controllers to access the uSD slot. You can use either. The other controller gets routed to the SDIO card. The choice made will impact ACPI OSes booted (e.g. Windows 10). Arasan, being an SDIO controller, is usually used with the WiFi adapter where available. SDHost cannot be used with SDIO. In UEFI setup screen:
- go to `Device Manager`
- go to `Raspberry Pi Configuration`
- go to `Chipset`
- configure `Boot uSD Routing`

Known issues:
- Arasan HS/4bit support is missing.
- No 8 bit mode support for (e)MMC (irrelevant for the Pi 3).
- Hacky (e)MMC support (no HS).
- No card removal/replacement detection, tons of timeouts and slow down during boot without an uSD card present.

## USB

- USB1 BBB mass storage devices untested (USB2 and USB3 devices are fine).
- USB1 CBI mass storage devices don't work (e.g. HP FD-05PUB floppy).

## ACPI

More-or-less matches MS-IoT ones. Good enough to boot WinPE, but unclear how functional all of it is, given current state of WoA on RPi3. Both Arasan and SDHost SD controllers are exposed.

# Missing Functionality

- Network booting via onboard NIC.
- Ability to switch UART use to PL011.

# Licensing

All of the code is BSD licensed, with the exception of:
1) `DwUsbHostDxe`, which I've documented as being GPL 2.0 licensed, since
   it appears to be directly related to the U-Boot driver.

# Contact

Andrey Warkentin <andrey.warkentin@gmail.com>

Btw, feel free to upstream, if so inclined.
