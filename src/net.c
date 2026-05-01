/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Common networking code
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
#include <rtems/ntpd.h>
#include <rtems/rtems/tasks.h>
#include <rtems/telnetd.h>
#include <cexp.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "rtems-init.h"
#include "common/util.h"

static rtems_id ntp_thread;

static void telnetd_init_command(char*, void*);

/****************************************************************************\
 * Telnet daemon
\****************************************************************************/

/* Note: Must be global symbol */
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

  int fd = fileno(stdin);
  ios_shell_input(fd, NULL);

  struct termios tio;
  if (tcgetattr(fd, &tio) >= 0) {
    tio.c_lflag |= ECHO;
    tcsetattr(fd, TCSANOW, &tio);
  }

  #if RTI_CONFIG_LOGIN_SHELL == RTI_SH_RTSH
  {
    rtems_shell_dup_current_env(&se);
    se.devname = dev;
    se.taskname = "TELN";
    se.forever = false;
    se.login_check = NULL;
    rtems_shell_main_loop(&se);
  }
  #elif RTI_CONFIG_LOGIN_SHELL == RTI_SH_CEXP
    cexpsh(NULL);
  #endif
}

int
generate_resolv_conf()
{
  FILE* fp = fopen("/etc/resolv.conf", "wb");
  if (!fp) {
    kerror("Failed to generate /etc/resolv.conf\n");
    return -1;
  }

  const char* d = NULL;
  if ((d = getenv("BP_DNS1")))
    fprintf(fp, "nameserver %s\n", d);
  if ((d = getenv("BP_DNS2")))
    fprintf(fp, "nameserver %s\n", d);
  if ((d = getenv("BP_DNS3")))
    fprintf(fp, "nameserver %s\n\n", d);
  if ((d = getenv("BP_MYDN")))
    fprintf(fp, "search %s\n", d);

  fclose(fp);
  return 0;
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
    kwarn("No NTP servers provided by DHCP or NVRAM, skipping NTP...\n");
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

  klog("NTP servers: %s %s %s\n", 
    pntp1 ? pntp1 : "",
    pntp2 ? pntp2 : "",
    pntp3 ? pntp3: "");

  r = rtems_ntpd_run(i, ntpcmd);
  
  if (r != 0) {
    kerror("ntpd exited abnormally\n");
  }

  klog("ntpd exited\n");
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

  klog("Set timezone to %s\n", buf);
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
    kerror("Failed to create NTPD task\n");
  }
  else {
    if (rtems_task_start(ntp_thread, run_ntpd, 0) != RTEMS_SUCCESSFUL) {
      kerror("Failed to start NTPD task\n");
    }
  }

  return 0;
}

void
ntp_shutdown()
{
  rtems_ntpd_stop();
}

