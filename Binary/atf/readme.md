ARM Trusted Firmware for RPi3
=============================

Last updated May 9th, 2018.

These are binary deliverables for a basic RPi3 ATF, equivalent
to https://github.com/andreiw/raspberry-pi3-atf/commit/397151d6939cd661d84b2dd095a978d81a742b10

If you want to use your own build, simply override ATF_BUILD_DIR, e.g.:
```
build -p RaspberryPiPkg/RaspberryPiPkg.dsc -D ATF_BUILD_DIR=$PWD/raspberry-pi3-atf/build/rpi3/debug
```
