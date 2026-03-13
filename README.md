# rtems-init

[doi: 10.11578/dc.20260312.1](https://doi.org/10.11578/dc.20260312.1)

An init system and testing framework for RTEMS applications.

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

The following BSPs have been tested with rtems-init:
- powerpc/beatnik (MVME6100, MVME5500)
- powerpc/mvme3100
- m68k/uC5282
- i386/pc686 (QEMU)

## Simulating with QEMU

rtems-init can run on QEMU using the pc686-qemu target. Other targets may work, but I haven't tested them.

First, build the target using `ninja -C build-cmake/build-rtems7-pc686-qemu`

Run with `./tests/run-qemu-i386.sh`

This requires that you have `qemu-system-i386` installed on your system.

## Components

rtems-init is largely a rewrite of ssrlApps and thus uses many of the ssrlApps modules. Additional test code and standalone apps are also provided.

File structure:
- `modules/`
    - Contains modules rtems-init can use.
- `rootfs/`
    - Contains the skeleton rootfs that is built into the image. Includes directories like etc/ for configs.
- `src/`
    - Source code for the rtems-init application.
    - `apps/`
        - Standalone test applications, usually including BSP-specific test behavior.
    - `fdt/`
        - Device tree files.
    - `common/`
        - Common code. This can be used in a standalone manner and does not depend on rtems-init.
    - `test/`
        - Test code that is integrated directly into rtems-init.
- `docs/`
    - Sphinx documentation.
- `tools/`
    - rtems-tools submodule. Helper scripts and whatnot.

### Loadable Modules

These modules are loadable separately from the rtems-init application itself. 

- modules/miscUtils
  - Implements misc utilities that are useful to RTEMS applications. Includes things like extra commands and tools.
- modules/bspExt
  - Extensions to the legacy-style RTEMS ISR API. Retained for compatibility with older RTEMS apps.
- modules/drvLan9118 (soon)
  - Raw UDP packet driver for the LCLS fast feedback system 
  
### Built-in Modules

These components are linked into the rtems-init application itself, and are always available.

- modules/cexp
  - C expression shell, legacy component originally built by SLAC for RTEMS to emulate the vxWorks shell.
- modules/lua
  - RTEMS port of Lua, submoduled in.

# Copyright Notice
            
COPYRIGHT © SLAC National Accelerator Laboratory. All rights reserved. This work is supported [in part] by the U.S. Department of Energy, Office of Basic Energy Sciences under contract DE-AC02-76SF00515.

# Usage Restrictions

Neither the name of the Leland Stanford Junior University, SLAC National Accelerator Laboratory, U.S. Department of Energy nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

