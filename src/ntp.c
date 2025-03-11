
#include <rtems.h>
#include <rtems/ntpd.h>
#include <rtems/ntpq.h>
#include <rtems/shell.h>
#include <rtems/bsd/bsd.h>
#include <rtems/bsd.h>
#include <rtems/bsd/iface.h>
#include <rtems/bsd/modules.h>
#include <machine/rtems-bsd-commands.h>

#include "rtems-init.h"
#include "util.h"

static rtems_id ntp_thread;

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
