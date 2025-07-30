# rtems-init+.

rtems-init performs necessary setup (DHCP, NTP), and provides a framework for system configuration.
This is mostly targeted at EPICS IOCs, particularly ones that need additional system configuration outside of EPICS base itself.

## Configuring

Makefile.cmake is a utility makefile that will configure the build for you.
```
make -f Makefile.cmake TARGETS="rtems7-uC5282 rtems7-beatnik rtems7-mvme3100" PREFIX=/sdf/group/cds/sw/epics/users/lorelli/rtems/7.0
```

## Boot Process

Boot parameters are supplied by two sources: NVRAM and DHCP. Parameters are set in the environment and at the bspcmdline by netboot.

* rc.lua is executed at boot, if it exists

## Supported BSPs

Tested on: RTEMS-beatnik, RTEMS-pc686-qemu

## Simulating with QEMU

rtems-init can run on QEMU using the RTEMS-pc686-qemu target. Other targets may work, but I haven't tested them.

First, build the target using `ninja -C build-cmake/build-rtems6-pc686-qemu`

Run with `./tests/run-qemu-i386.sh`

This requires that you have `qemu-system-i386` installed on your system.

# Components

rtems-init (TODO RENAME ME!) is derived from ssrlApps.

Some of the shared components:
- libBspExt
  - Extensions to the RTEMS ISR API
- drvLan9118
  - Raw UDP packet driver for the LCLS fast feedback system 
