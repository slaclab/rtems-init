/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Configuration for RTEMS
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
#include <rtems.h>

extern void *POSIX_Init(void *argument);

/****************************************************************************\
 * RTEMS configuration
\****************************************************************************/

/** Match value in EPICS base (64): */
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 64
#define CONFIGURE_IMFS_ENABLE_MKFIFO    2

#define CONFIGURE_MAXIMUM_NFS_MOUNTS 		3
#define CONFIGURE_MAXIMUM_USER_EXTENSIONS 	5

#define CONFIGURE_UNLIMITED_ALLOCATION_SIZE 32
#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#define CONFIGURE_STACK_CHECKER_ENABLED

#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK
#define CONFIGURE_BDBUF_BUFFER_MAX_SIZE (64 * 1024)
#define CONFIGURE_BDBUF_MAX_READ_AHEAD_BLOCKS 4
#define CONFIGURE_BDBUF_CACHE_MEMORY_SIZE (1 * 1024 * 1024)

#define CONFIGURE_POSIX_INIT_THREAD_TABLE
#define CONFIGURE_POSIX_INIT_THREAD_ENTRY_POINT POSIX_Init
#define CONFIGURE_POSIX_INIT_THREAD_STACK_SIZE  (64*1024)

#define CONFIGURE_MAXIMUM_PERIODS 	5
#define CONFIGURE_MICROSECONDS_PER_TICK 10000
#define CONFIGURE_MALLOC_STATISTICS     1
/* MINIMUM_STACK_SIZE == 8K */
#define CONFIGURE_MINIMUM_TASK_STACK_SIZE   (65536)
#define CONFIGURE_EXTRA_TASK_STACKS         (512 * RTEMS_MINIMUM_STACK_SIZE)

/** Enable various filesystem backends */;
#define CONFIGURE_FILESYSTEM_DEVFS
#define CONFIGURE_FILESYSTEM_TFTPFS
#ifndef RTEMS_LEGACY_STACK
#define CONFIGURE_FILESYSTEM_NFS
#endif
#define CONFIGURE_FILESYSTEM_IMFS

/****************************************************************************\
 * libbsd configuration
\****************************************************************************/

#define RTEMS_BSD_CONFIG_BSP_CONFIG
#define RTEMS_BSD_CONFIG_SERVICE_TELNETD
#define RTEMS_BSD_CONFIG_TELNETD_STACK_SIZE (16 * 1024)
#define RTEMS_BSD_CONFIG_FIREWALL_PF

#ifndef RTEMS_LEGACY_STACK

#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM
//#define RTEMS_BSD_CONFIG_NET_PF_UNIX
//#define RTEMS_BSD_CONFIG_NET_IF_BRIDGE
//#define RTEMS_BSD_CONFIG_NET_IF_LAGG
//#define RTEMS_BSD_CONFIG_NET_IF_VLAN
#define RTEMS_BSD_CONFIG_BSP_CONFIG
#define RTEMS_BSD_CONFIG_INIT
#include <machine/rtems-bsd-config.h>

#endif // !RTEMS_LEGACY_STACK

/****************************************************************************\
 * Shell commands
\****************************************************************************/

#define CONFIGURE_SHELL_COMMANDS_INIT

#include <bsp/irq-info.h>

#ifndef RTEMS_LEGACY_STACK
#include <rtems/netcmds-config.h>

#define CONFIGURE_SHELL_USER_COMMANDS \
  &bsp_interrupt_shell_command, \
  &rtems_shell_HOSTNAME_Command, \
  &rtems_shell_PING_Command, \
  &rtems_shell_ROUTE_Command, \
  &rtems_shell_NETSTAT_Command, \
  &rtems_shell_IFCONFIG_Command, \
  &rtems_shell_TCPDUMP_Command, \
  &rtems_shell_PFCTL_Command, \
  &rtems_shell_NTPQ_Command, \
  &rtems_shell_SYSCTL_Command, \
  &rtems_shell_VMSTAT_Command, \
  &rtems_shell_MALLOC_INFO_Command, \
  &rtems_shell_RTRACE_Command, \
  &rtems_shell_ARP_Command
#else // LEGACY_STACK:
#define CONFIGURE_SHELL_USER_COMMANDS \
  &bsp_interrupt_shell_command
#endif // not LEGACY_STACK

#define CONFIGURE_SHELL_COMMAND_CPUUSE
#define CONFIGURE_SHELL_COMMAND_PERIODUSE
#define CONFIGURE_SHELL_COMMAND_STACKUSE
#define CONFIGURE_SHELL_COMMAND_PROFREPORT
#define CONFIGURE_SHELL_COMMAND_TOP
#define CONFIGURE_SHELL_COMMAND_RTEMS

#define CONFIGURE_SHELL_COMMANDS_ALL_NETWORKING

#define CONFIGURE_SHELL_COMMANDS_ALL
#define CONFIGURE_SHELL_COMMAND_CP
#define CONFIGURE_SHELL_COMMAND_PWD
#define CONFIGURE_SHELL_COMMAND_LS
#define CONFIGURE_SHELL_COMMAND_LN
#define CONFIGURE_SHELL_COMMAND_LSOF
#define CONFIGURE_SHELL_COMMAND_CHDIR
#define CONFIGURE_SHELL_COMMAND_CD
#define CONFIGURE_SHELL_COMMAND_MKDIR
#define CONFIGURE_SHELL_COMMAND_RMDIR
#define CONFIGURE_SHELL_COMMAND_CAT
#define CONFIGURE_SHELL_COMMAND_MV
#define CONFIGURE_SHELL_COMMAND_RM
#define CONFIGURE_SHELL_COMMAND_MALLOC_INFO
#define CONFIGURE_SHELL_COMMAND_SHUTDOWN

#include <rtems/shellconfig-net-services.h>
#include <rtems/shellconfig.h>

/****************************************************************************\
 * Drivers + RTEMS confdefs
\****************************************************************************/

#define CONFIGURE_MAXIMUM_DRIVERS 40

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_STUB_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_ZERO_DRIVER

/** uC5282 has no RTC driver */
#ifndef BSP_uC5282
#define CONFIGURE_APPLICATION_NEEDS_RTC_DRIVER
#endif

#if defined(BSP_pc386) || defined(BSP_pc686)
#define RTEMS_BSD_CONFIG_DOMAIN_PAGE_MBUFS_SIZE (64 * 1024 * 1024)
#elif defined(BSP_qoriq_e500)
#define RTEMS_BSD_CONFIG_DOMAIN_PAGE_MBUFS_SIZE (32 * 1024 * 1024)
#endif

#define CONFIGURE_INIT

#include <rtems/confdefs.h>
