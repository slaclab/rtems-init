/**
 * Misc. shell utilities
 */
#include <rtems.h>
#include <rtems/shell.h>
#include <dlfcn.h>
#include <rtems/rtl/dlfcn-shell.h>
#include <rtems/rtl/rtl-shell.h>
#include <rtems/ntpq.h>
#include <rtems/ntpd.h>
#include <rtems/rtems-debugger.h>
#include <rtems/rtems-debugger-remote-tcp.h>
#include <machine/rtems-bsd-commands.h>
#include <unistd.h>
#include <fcntl.h>

#include "rtems-init.h"
#include "getopt_s.h"
#include "util.h"

static int shell_debugger_start(int,char**);
static int shell_debugger_stop(int argc, char** argv);
static int shell_read_temp(int argc, char** argv);

#define GEV_SHOW_USAGE "gevShow -- Display all GEV NVRAM parameters"
#define GEV_GET_USAGE "gevGet paramName -- Display value of a specific GEV parameter"
#define NVRAM_SHOW_USAGE "nvramShow -- Display all NVRAM boot parameters"
#define NVRAM_GET_USAGE "nvramGet paramName -- Display value of specific NVRAM boot parameter"

struct shell_cmd shell_cmds[] =
{
  /* cmd, topic, usage, func */
  { "dlopen",     "rtl",    "",                   shell_dlopen },
  { "dlsym",      "rtl",    "",                   shell_dlsym },
  { "dlclose",    "rtl",    "",                   shell_dlclose },
  { "dlsym",      "rtl",    "",                   shell_dlcall },
  { "rtl",        "rtl",    "",                   rtems_rtl_shell_command },
  { "dbgstart",   "misc",   "",                   shell_debugger_start },
  { "dbgstop",    "misc",   "",                   shell_debugger_stop },
  { "nvramGet",   "nvram",  NVRAM_GET_USAGE,      shell_nvram_get },
  { "nvramShow",  "nvram",  NVRAM_SHOW_USAGE,     shell_nvram_show },
  { "gevGet",     "nvram",  GEV_GET_USAGE,        shell_gev_get },
  { "gevShow",    "nvram",  GEV_SHOW_USAGE,       shell_gev_show },
  { "temp",       "misc",   "",                   shell_read_temp },
  { "ntpd",       "net",    "",                   rtems_ntpd_run },
  { NULL,         NULL,     NULL,                 NULL },
};

static int
shell_debugger_start(int argc, char** argv)
{
  int opt = -1;
  char port[32] = "1234";

  struct getopt_state s;
  getopt_state_init(&s);
  while ((opt = getopt_s(opt, argv, "p:h", &s)) != -1) {
    switch (opt) {
    case 'p':
      strncpySafe(port, optarg, sizeof(port));
      break;
    default:
    case 'h':
      fprintf(stderr, "USAGE: %s [-p port] [-t timeout]\n", argv[0]);
      return -1;
    }
  }

  rtems_debugger_register_tcp_remote();
  rtems_printer printer;
  rtems_print_printer_printf(&printer);
  if (rtems_debugger_start("tcp", port, RTEMS_DEBUGGER_TIMEOUT, 1, &printer) != 0) {
    fprintf(stderr, "Failed to start debugger\n");
    return -1;
  }
  return 0;
}

static int
shell_debugger_stop(int argc, char** argv)
{
  if (!rtems_debugger_running()) {
    fprintf(stderr, "debugger is not running\n");
    return -1;
  }
  rtems_debugger_stop();
  return 0;
}


static int
shell_read_temp(int argc, char** argv)
{
#ifdef BSP_beatnik
  int fd;
  if ((fd = open("/dev/i2c0.ds1621", O_RDONLY)) < 0) {
    perror("failed to open /dev/i2c0.ds1621");
    return -1;
  }
  
  char buf[128];
  if (read(fd, buf, sizeof(buf)) < 0)
    perror("read failed");
  else
    puts(buf);

  close(fd);
#else
  printf("temperature reading not supported on this BSP\n");
#endif
  return 0;
}