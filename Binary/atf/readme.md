ARM Trusted Firmware for RPi3
=============================

Last updated May 22nd, 2018.

These are binary deliverables for a basic RPi3 ATF, equivalent
to https://github.com/andreiw/raspberry-pi3-atf/commit/33bffd16af01dddc01360e7b81ad4ec82e294b9f

If you want to use your own build, simply override ATF_BUILD_DIR, e.g.:
```
build -p RaspberryPiPkg/RaspberryPiPkg.dsc -D ATF_BUILD_DIR=$PWD/raspberry-pi3-atf/build/rpi3/debug
```
