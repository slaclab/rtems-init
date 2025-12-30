/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Configuration options for rtems-init
 * ----------------------------------------------------------------------------
 * This file is part of 'rtems-init'. It is subject to the license terms in the
 * LICENSE.txt file found in the top-level directory of this distribution,
 * and at:
 *    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
 * No part of 'rtems-init', including this file, may be copied, modified,
 * propagated, or distributed except according to the terms contained in the
 * LICENSE.txt file.
 * ----------------------------------------------------------------------------
 **/
#pragma once

#include <bsp.h>

#define RTI_SH_RTSH 0    /* RTEMS shell */
#define RTI_SH_CEXP 1    /* Cexpsh */

/* Amount of time the early-shell abort prompt will show for.
 * This shows on every boot to prompt the user to enter an early debug shell,
 * before a bulk of system init is done.
 * Set to 0 to skip entirely. */
#define RTI_CONFIG_EARLYSHELL_TIMEOUT 5

/* When set, skip network initialization. Handy for quick debugging on BSPs
 * that don't have a functioning network stack (i.e. in the process of porting
 * libbsd to the BSP) */
#undef RTI_CONFIG_SKIP_NETWORK

/* When set, skip DHCP initialization. Handy for quick debugging BSPs */
#undef RTI_CONFIG_SKIP_DHCP

/* Set the default login shell. May be either RTEMS shell or Cexpsh, defaults
 * to Cexpsh */
#define RTI_CONFIG_LOGIN_SHELL RTI_SH_CEXP

/* Run tests after initialization is complete. Handy for quick debugging */
#undef RTI_CONFIG_TESTS_ON_BOOT

/*--------------------- BSP specific configuration below --------------------*/

#ifdef LIBBSP_POWERPC_MVME3100_BSP_H      /* MVME-3100 (PowerPC e500) */

# define HAVE_NVRAM
# define HAVE_MOTLOAD

# define RTI_CONFIG_SKIP_NETWORK 1
# undef  RTI_CONFIG_LOGIN_SHELL
# define RTI_CONFIG_LOGIN_SHELL RTI_SH_RTSH

#elif defined(LIBBSP_BEATNIK_BSP_H)       /* MVME-6100/MVME-5500 (PPC G4) */

# define HAVE_NVRAM
# define HAVE_MOTLOAD

#elif defined(LIBBSP_I386_PC386_BSP_H)    /* Any x86 (usually QEMU) */

/* Nothing to do */

#elif defined(LIBBSP_M68K_UC5282_BSP_H)   /* uC5282 (Coldfire v2, m68k) */

# define RTEMS_NETWORK_CONFIG_MBUF_SPACE 200
# define RTEMS_NETWORK_CONFIG_CLUSTER_SPACE 350
# define HAVE_NVRAM
# define CONFIGURE_MINIMUM_TASK_STACK_SIZE 8192
# define CONFIGURE_EXTRA_TASK_STACKS (32 * RTEMS_MINIMUM_STACK_SIZE)

#else

#error "Unsupported BSP! Add your BSP configuration below!"

#endif

/*---------------------------------------------------------------------------*/

#include "config.h"

#if !defined(HAVE_CEXP) && RTI_CONFIG_LOGIN_SHELL == RTI_SH_CEXP
# warning "We don't have Cexp, but RTI_CONFIG_LOGIN_SHELL is Cexp. Defaulting to RTEMS shell"
# undef RTI_RTI_CONFIG_LOGIN_SHELL
# define RTI_RTI_CONFIG_LOGIN_SHELL RTI_SH_RTSH
#endif