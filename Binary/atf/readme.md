ARM Trusted Firmware for RPi3
=============================

Last updated December 6th, 2017.

These are binary deliverables for a basic RPi3 ATF, equivalent
to https://github.com/andreiw/raspberry-pi3-atf/commit/39829ca5c048a217b1e725d4efdad8861596250c

If you want to use your own build, simply override ATF_BUILD_DIR, e.g.:
```
build -p RaspberryPiPkg/RaspberryPiPkg.dsc -D ATF_BUILD_DIR=$PWD/raspberry-pi3-atf/build/rpi3/debug
```
