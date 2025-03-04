# rtems-init+.

rtems-init performs necessary setup (DHCP, NTP), and provides a framework for system configuration.
This is mostly targeted at EPICS IOCs, particularly ones that need additional system configuration outside of EPICS base itself.

## Supported BSPs

Tested on: RTEMS-beatnik, RTEMS-pc686-qemu
