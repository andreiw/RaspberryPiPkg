ARM Trusted Firmware for RPi3
=============================

Last updated July 8th, 2018.

These are binary deliverables for a basic RPi3 ATF, equivalent
to https://github.com/andreiw/raspberry-pi3-atf/commit/8e83d96b19d75cd19d683fc01cd0d538da90629d

If you want to use your own build, simply override ATF_BUILD_DIR, e.g.:
```
build -p RaspberryPiPkg/RaspberryPiPkg.dsc -D ATF_BUILD_DIR=$PWD/raspberry-pi3-atf/build/rpi3/debug
```
