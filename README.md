# rtems-init+.

rtems-init performs necessary setup (DHCP, NTP), and provides a framework for system configuration.
This is mostly targeted at EPICS IOCs, particularly ones that need additional system configuration outside of EPICS base itself.

## Supported BSPs

Tested on: RTEMS-beatnik, RTEMS-pc686-qemu

## Running with QEMU

The target that supports qemu is RTEMS-pc686-qemu, although you may have success with other targets.

First, build the target using `ninja -C build-cmake/build-rtems6-pc686-qemu`

Run with `./tests/run-qemu-i386.sh`

This requires that you have `qemu-system-i386` installed on your system.
