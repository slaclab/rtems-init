# Supported Hardware

## Support Matrix

| Board Name | BSP      | Networking Stack | Notes |
|------------|----------|------------------|-------|
| mvme6100   | beatnik  | libbsd           | |
| mvme3100   | mvme3100 | libbsd           | `tsec` network driver requires libbsd patches |
| mvme5500   | beatnik  | libbsd           | `em` network driver currently does not work |
| uC5282     | uC5282   | legacy           | |
| pc686      | i386     | libbsd           | For testing in Qemu |
| K26 SOM    | zynqmp-api | libbsd         | Tested on Xilinx Kv260 devkit (w/K26 SOM) |


## BSP Preprocessor Checks

These are defined in the individual bsp.h headers from RTEMS itself.

|     BSP         |             Macro                     |
|-----------------|---------------------------------------|
| beatnik         | `LIBBSP_BEATNIK_BSP_H`                |
| mvme3100        | `LIBBSP_POWERPC_MVME3100_BSP_H`       |
| mvme5500        | `LIBBSP_BEATNIK_BSP_H`                |
| uC5282          | `LIBBSP_M68K_UC5282_BSP_H`            |
| pc686           | `LIBBSP_I386_PC386_BSP_H`             |
| amd-k26/zynqmp  | `LIBBSP_AARCH64_XILINX_ZYNQMP_BSP_H ` |

## Other Feature Checks

These are set by our wscript and CMake scripts.

| Define | Description |
|--------|-------------|
| `RTEMS_LEGACY_STACK`      | Set when we have the legacy networking stack |
| `RTEMS_BSD_STACK`         | Set when we have the BSD networking stack |
| `HAVE_RTEMS_H`            | |
| `HAVE_SYS_MMAN_H`         | |
| `HAVE_STRINGS_H`          | |
| `HAVE_SYS_SELECT_H`       | |
| `HAVE_SYS_TERMIOS_H`      | |
| `HAVE_TERMIOS_H`          | |
| `HAVE_NCURSES_TERM_H`     | |
| `HAVE_NCURSES_CURSES_H`   | |
| `HAVE_SYS_FEATURES_H`     | |
| `HAVE_LINK_H`             | |
| `HAVE_PTHREADS`           | |
| `HAVE_RTEMS_CACHE_H`      | |
| `HAVE_DEBUGGER`           | |
| `HAVE_BSD_NETWORKING`     | |
| `HAVE_LEGACY_NETWORKING`  | |
| `HAVE_PCI`                | |
