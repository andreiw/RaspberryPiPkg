ARM Trusted Firmware for RPi3
=============================

Last updated February 27th, 2018.

These are binary deliverables for a basic RPi3 ATF, equivalent
to https://github.com/andreiw/raspberry-pi3-atf/commit/06f3f9ef8165fb13cb2a2652eb3f4d6c28fbf5d9

If you want to use your own build, simply override ATF_BUILD_DIR, e.g.:
```
build -p RaspberryPiPkg/RaspberryPiPkg.dsc -D ATF_BUILD_DIR=$PWD/raspberry-pi3-atf/build/rpi3/debug
```
