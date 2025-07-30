/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: BSD networking stack init code
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
#include <rtems/bsd.h>
#include <rtems/bsd/bsd.h>
#include <rtems/telnetd.h>
#include <rtems/dhcpcd.h>
#include <rtems/bsd/util.h>
#include <machine/rtems-bsd-commands.h>
#include <rtems/ntpd.h>
#include <syslog.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "rtems-init.h"
#include "util.h"

static void telnetd_init_command(char*, void*);

/****************************************************************************\
 * Telnet daemon
\****************************************************************************/

rtems_telnetd_config_table rtems_telnetd_config = {
  .stack_size = 0,
  .login_check = NULL,
  .client_maximum = 0,
  .port = 5512,
  .keep_stdio = 0,
  .command = telnetd_init_command,
};

/**
 * Create new shell session for telnet
 */
static void
telnetd_init_command(char* dev, void* cfg)
{
  rtems_shell_env_t se;

  rtems_shell_dup_current_env(&se);
  se.devname = dev;
  se.taskname = "TELN";
  se.forever = false;
  se.login_check = NULL;

  rtems_shell_main_loop(&se);
}

/****************************************************************************\
 * DHCP configuration
\****************************************************************************/

void
dhcpcd_hook_handler(struct rtems_dhcpcd_hook* h, char* const* env)
{
  int bound = 0;
  char* c = NULL;

  for (char* const* e = env; *e != NULL; ++e) {
    if (verbose)
      klog(" dhcpd: '%s'\n", *e);

    if (strHasPrefix(*e, "interface")) {
      if ((c = strpbrk(*e, "="))) {
        ++c;
        strncpySafe(dhcp_runtime_cfg.interface, c,
          sizeof(dhcp_runtime_cfg.interface));
      }
    }
    else if (strHasPrefix(*e, "reason")) {
      if ((c = strpbrk(*e, "="))) {
        ++c;
        /** FIXME: Is REBIND correct or not? */
        if (!strcasecmp(c, "BOUND") || !strcasecmp(c, "REBIND"))
          bound = 1;
      }
    }
    else if (strHasPrefix(*e, "new_host_name")) {
      if ((c = strpbrk(*e, "="))) {
        ++c;
        sethostname(c, strlen(c));
      }
    }
    else if (strHasPrefix(*e, "new_ntp_servers")) {
      if ((c = strpbrk(*e, "="))) {
        strncpySafe(dhcp_runtime_cfg.ntp1, ++c,
          sizeof(dhcp_runtime_cfg.ntp1));
        strtok(dhcp_runtime_cfg.ntp1, " ");

        c = strpbrk(c, " ");
        if (!c) continue;
        strncpySafe(dhcp_runtime_cfg.ntp2, ++c,
          sizeof(dhcp_runtime_cfg.ntp2));
        strtok(dhcp_runtime_cfg.ntp2, " ");

        c = strpbrk(c, " ");
        if (!c) continue;
        strncpySafe(dhcp_runtime_cfg.ntp3, ++c,
          sizeof(dhcp_runtime_cfg.ntp3));
        strtok(dhcp_runtime_cfg.ntp3, " ");
      }
    }
    else if (strHasPrefix(*e, "new_domain_name_servers")) {
      if ((c = strpbrk(*e, "="))) {
        strncpySafe(dhcp_runtime_cfg.dns1, ++c,
          sizeof(dhcp_runtime_cfg.dns1));
        strtok(dhcp_runtime_cfg.dns1, " ");

        c = strpbrk(c, " ");
        if (!c) continue;
        strncpySafe(dhcp_runtime_cfg.dns2, ++c,
          sizeof(dhcp_runtime_cfg.dns2));
        strtok(dhcp_runtime_cfg.dns2, " ");

        c = strpbrk(c, " ");
        if (!c) continue;
        strncpySafe(dhcp_runtime_cfg.dns3, ++c,
          sizeof(dhcp_runtime_cfg.dns3));
        strtok(dhcp_runtime_cfg.dns3, " ");
      }
    }
    else if (strHasPrefix(*e, "new_domain_name")) {
      strncpySafe(dhcp_runtime_cfg.domain, *e, 
        sizeof(dhcp_runtime_cfg.domain));
    }
    else if (strHasPrefix(*e, "new_tftp_server_name")) {
      if ((c = strpbrk(*e, "="))) {
        ++c;
        strncpySafe(dhcp_runtime_cfg.tftp_server, c,
          sizeof(dhcp_runtime_cfg.tftp_server));
      }
    }
    else if (strHasPrefix(*e, "new_bootfile_name")) {
      if ((c = strpbrk(*e, "="))) {
        ++c;
        strncpySafe(dhcp_runtime_cfg.bootfile, c,
          sizeof(dhcp_runtime_cfg.bootfile));
      }
    }
    else if (strHasPrefix(*e, "new_filename")) {
      if ((c = strpbrk(*e, "="))) {
        ++c;
        strncpySafe(dhcp_runtime_cfg.filename, c,
          sizeof(dhcp_runtime_cfg.filename));
      }
    }
    else if (strHasPrefix(*e, "new_rtems_cmdline")) {
      if ((c = strpbrk(*e, "="))) {
        ++c;
        strncpySafe(dhcp_runtime_cfg.cmdline, c,
          sizeof(dhcp_runtime_cfg.cmdline));
      }
    }
    else if (strHasPrefix(*e, "posix_timezone")) {
      if ((c = strpbrk(*e, "="))) {
        ++c;
        strncpySafe(dhcp_runtime_cfg.timezone, c,
          sizeof(dhcp_runtime_cfg.timezone));
      }
    }
  }
  
  if (bound) {
    printf("dhcp: done\n");

    /** Commit changes to environment */
    if (init_mode == INIT_MODE_DHCP) {
      setenv("BP_DNS1", dhcp_runtime_cfg.dns1, 1);
      setenv("BP_DNS2", dhcp_runtime_cfg.dns2, 1);
      setenv("BP_DNS3", dhcp_runtime_cfg.dns3, 1);
      setenv("BP_NTP1", dhcp_runtime_cfg.ntp1, 1);
      setenv("BP_NTP2", dhcp_runtime_cfg.ntp2, 1);
      setenv("BP_NTP3", dhcp_runtime_cfg.ntp3, 1);
      setenv("BP_PARM", dhcp_runtime_cfg.cmdline, 1);
      setenv("BP_MYDN", dhcp_runtime_cfg.domain, 1);
      setenv("BP_FILE", dhcp_runtime_cfg.bootfile, 1);
    }

    event_signal(dhcp_runtime_cfg.event);
  }
}

static rtems_dhcpcd_hook dhcpcd_hook = {
  .handler = dhcpcd_hook_handler,
  .name = "rtems-init"
};

static char* dhcpcd_args[] = {
  "dhcpcd",
  "--vendorclassid=udhcp",
  NULL
};

static void
do_dhcp()
{
  dhcp_runtime_cfg.event = event_create();
  assert(dhcp_runtime_cfg.event);

  /** Start dhcpcd for network configuration */
  rtems_dhcpcd_add_hook(&dhcpcd_hook);
  rtems_dhcpcd_start(NULL);

  if (event_wait(dhcp_runtime_cfg.event, 300 * 1000) != ETIMEDOUT)
    event_destroy(dhcp_runtime_cfg.event);
  /** FIXME: timeout leaks an event */
}

static int
bsd_vprintf_logger(int sevr, const char* fmt, va_list va)
{
  int r;
  switch (sevr) {
  case LOG_WARNING:
    r = kvwarn(fmt, va);
    break;
  case LOG_CRIT:
  case LOG_ERR:
    r = kverror(fmt, va);
    break;
  default:
    r = kvlog(fmt, va);
    break;
  }
  fputc('\n', stderr);
  return r;
}

/****************************************************************************\
 * Network init
\****************************************************************************/

void
network_init()
{
  int r;
  klog("Starting BSD networking stack\n");

  // From EPICS base:
#if defined(__i386__)
  // glorious hack to stub out useless EEPROM check
  // which takes sooooo longggg w/ QEMU
  // Writes a 'ret' instruction to immediatly return to the caller
  extern void _bsd_e1000_validate_nvm_checksum(void);
  *(char*)&_bsd_e1000_validate_nvm_checksum = 0xc3;
#endif

  if (rtems_bsd_initialize() != RTEMS_SUCCESSFUL) {
    kerror("network_init: rtems_bsd_initialize failed\n");
    abort();
    return;
  }

  rtems_bsd_setlogpriority("debug");
  rtems_bsd_set_vprintf_handler(bsd_vprintf_logger);

  if (rtems_bsd_ifconfig_lo0() != 0) {
    kerror("network_init: rtems_bsd_ifconfig_lo0 failed\n");
    abort();
    return;
  }
  
  if (!bsp_cmdline_has_param("--nodhcp"))
    do_dhcp();
  else {
    klog("Skipping dhcp per request\n");

    /** QEMU specific configuration */
    char* ifcmd[] = {
      "ifconfig",
      "em0",
      "inet",
      "10.0.2.15",
      "netmask",
      "255.255.255.0",
      NULL
    };

    if (rtems_bsd_command_ifconfig(RTEMS_BSD_ARGC(ifcmd), ifcmd) 
          != EXIT_SUCCESS) {
      kerror("network_init: rtems_bsd_command_ifconfig failed\n");
    }
  }
  
  /** Display current configuration */
  static char* IFCONFIG_ARGS[] = {"ifconfig", NULL};
  rtems_bsd_command_ifconfig(1, IFCONFIG_ARGS);
  
  klog("Generating /etc/resolv.conf\n");
  generate_resolv_conf();

  klog("Starting ntpd\n");

  if (ntp_init() != 0) {
    kerror("NTP init failed; it will now be disabled\n");
  }

  klog("Starting telnetd\n");
  if (rtems_telnetd_initialize() != RTEMS_SUCCESSFUL) {
    kerror("Failed to init telnetd\n");
  }
  else {
    if (rtems_telnetd_start(&rtems_telnetd_config) != RTEMS_SUCCESSFUL) {
      kerror("Failed to start telnetd\n");
    }
  }

  klog("End BSD network init\n");
}
