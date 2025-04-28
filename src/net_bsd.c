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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "rtems-init.h"
#include "util.h"

static void telnetd_init_command(char*, void*);

static rtems_id ntp_thread;

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
      printf(" dhcpd: '%s'\n", *e);

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

static rtems_dhcpcd_config dhcpcd_config = {
  .argc = RTEMS_BSD_ARGC(dhcpcd_args),
  .argv = dhcpcd_args,
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

/****************************************************************************\
 * NTPD configuration
\****************************************************************************/

static rtems_task
run_ntpd(rtems_task_argument arg)
{
  char* pntp1 = NULL, *pntp2 = NULL, *pntp3 = NULL;
  
  /** Use dhcp if available */
  if (*dhcp_runtime_cfg.ntp1)
    pntp1 = dhcp_runtime_cfg.ntp1;
  if (*dhcp_runtime_cfg.ntp2)
    pntp1 = dhcp_runtime_cfg.ntp2;
  if (*dhcp_runtime_cfg.ntp3)
    pntp1 = dhcp_runtime_cfg.ntp3;

  /** NTP servers provided by nvram */
  static char ntp1[128], ntp2[128], ntp3[128];
  nvram_get_boot_param("BP_NTP1", ntp1, sizeof(ntp1));
  nvram_get_boot_param("BP_NTP2", ntp2, sizeof(ntp2));
  nvram_get_boot_param("BP_NTP3", ntp3, sizeof(ntp3));

  if (*ntp1 && !pntp1) pntp1 = ntp1;
  if (*ntp2 && !pntp2) pntp2 = ntp2;
  if (*ntp3 && !pntp3) pntp3 = ntp3;
  
  if (!pntp1) {
    printf("No NTP servers provided by DHCP or NVRAM, skipping NTP...\n");
    rtems_task_delete(RTEMS_SELF);
    return;
  }

  int r = 0;
  char* ntpcmd[] = {
    "ntpd",
    "-g",
    NULL, NULL, NULL,
    NULL
  };
  int i = 2;
  if (pntp1) ntpcmd[i++] = pntp1;
  if (pntp2) ntpcmd[i++] = pntp2;
  if (pntp3) ntpcmd[i++] = pntp3;

  printf("NTP servers: %s %s %s\n", 
    pntp1 ? pntp1 : "",
    pntp2 ? pntp2 : "",
    pntp3 ? pntp3: "");

  r = rtems_ntpd_run(i, ntpcmd);
  
  if (r != 0) {
    printf("ntpd exited abnormally\n");
  }

  printf("ntpd exited\n");
  rtems_task_delete(RTEMS_SELF);
}

void
tz_init()
{
  char buf[512];
  /** Use /etc/localtime if TZ not already supplied by dhcp */
  if (!getenv("TZ") && !*getenv("TZ")) {
    if (read_file("/etc/localtime", buf, sizeof(buf)) < 0) {
      printf("**** Could not read /etc/localtime\n");
      return;
    }
  }
  else {
    strncpySafe(buf, getenv("TZ"), sizeof(buf));
  }
  strtok(buf, ",");
  setenv("TZ", buf, 1);
  tzset();

  printf("**** Set timezone to %s\n", buf);
}

int
ntp_init()
{
  rtems_status_code r;

  tz_init();

  rtems_name ntpt = rtems_build_name('N', 'T', 'P', 'D');
  r = rtems_task_create(
    ntpt,
    254,
    64*1024,
    RTEMS_TIMESLICE,
    RTEMS_FLOATING_POINT,
    &ntp_thread
  );
  
  if (r != RTEMS_SUCCESSFUL) {
    printf("Failed to create NTPD task\n");
  }
  else {
    if (rtems_task_start(ntp_thread, run_ntpd, 0) != RTEMS_SUCCESSFUL) {
      printf("Failed to start NTPD task\n");
    }
  }

  return 0;
}

void
ntp_shutdown()
{
  rtems_ntpd_stop();
}

/****************************************************************************\
 * Network init
\****************************************************************************/

void
network_init()
{
  int r;
  printf("** Begin BSD network init\n");

  // From EPICS base:
#if defined(__i386__)
  // glorious hack to stub out useless EEPROM check
  // which takes sooooo longggg w/ QEMU
  // Writes a 'ret' instruction to immediatly return to the caller
  extern void _bsd_e1000_validate_nvm_checksum(void);
  *(char*)&_bsd_e1000_validate_nvm_checksum = 0xc3;
#endif

  if (rtems_bsd_initialize() != RTEMS_SUCCESSFUL) {
    printf("*** rtems_bsd_initialize failed\n");
    abort();
    return;
  }

  rtems_bsd_setlogpriority("debug");
  
  if (rtems_bsd_ifconfig_lo0() != 0) {
    printf("*** rtems_bsd_ifconfig_lo0 failed\n");
    abort();
    return;
  }
  
  if (!bsp_cmdline_has_param("--nodhcp"))
    do_dhcp();
  else {
    printf("*** Skipping dhcp per request\n");

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
      printf("*** rtems_bsd_command_ifconfig failed\n");
    }
  }
  
  /** Display current configuration */
  static char* IFCONFIG_ARGS[] = {"ifconfig", NULL};
  rtems_bsd_command_ifconfig(1, IFCONFIG_ARGS);
  
  printf("*** Generating /etc/resolv.conf\n");
  generate_resolv_conf();

  printf("*** Starting ntpd\n");

  if (ntp_init() != 0) {
    printf("**** NTP init failed; it will now be disabled\n");
  }

  printf("*** Starting telnetd\n");
  if (rtems_telnetd_initialize() != RTEMS_SUCCESSFUL) {
    printf("**** Failed to init telnetd\n");
  }
  else {
    if (rtems_telnetd_start(&rtems_telnetd_config) != RTEMS_SUCCESSFUL) {
      printf("**** Failed to start telnetd\n");
    }
  }

  printf("** End BSD network init\n");
}
