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
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "rtems-init.h"
#include "getopt_s.h"
#include "util.h"

static int shell_debugger_start(int,char**);
static int shell_debugger_stop(int argc, char** argv);
static int shell_read_temp(int argc, char** argv);
static int shell_dumpenv(int argc, char** argv);
static int shell_setuid(int argc, char** argv);
static int shell_setgid(int argc, char** argv);
static int shell_getuid(int argc, char** argv);
static int shell_getaddrinfo(int argc, char** argv);
static int shell_apropos(int argc, char** argv);

#define GEV_SHOW_USAGE "gevShow -- Display all GEV NVRAM parameters"
#define GEV_GET_USAGE "gevGet paramName -- Display value of a specific GEV parameter"
#define NVRAM_SHOW_USAGE "nvramShow -- Display all NVRAM boot parameters"
#define NVRAM_GET_USAGE "nvramGet paramName -- Display value of specific NVRAM boot parameter"

/** !!! This is internal and I probably shouldn't do this! */
extern rtems_shell_cmd_t* rtems_shell_first_cmd;

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
  { "dumpenv",    "misc",   "",                   shell_dumpenv },
  { "setuid",     "misc",   "",                   shell_setuid },
  { "getuid",     "misc",   "",                   shell_getuid },
  { "setgid",     "misc",   "",                   shell_setgid },
  { "getaddrinfo","net",    "",                   shell_getaddrinfo },
  { "apropos",    "misc",   "",                   shell_apropos },
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

static int
shell_dumpenv(int argc, char** argv)
{
  for (char** e = environ; e && *e; e++) {
    puts(*e);
  }
  return 0;
}

static int
shell_setuid(int argc, char** argv)
{
  if (argc < 2) {
    printf("USAGE: %s <id>\n", argv[0]);
    return -1;
  }
  
  char* endp = NULL;
  uint32_t uid = strtoul(argv[1], &endp, 10);
  if (*endp) {
    printf("Invalid uid %s\n", argv[1]);
    return -1;
  }
  setuid(uid);
  
  return 0;
}

static int
shell_setgid(int argc, char** argv)
{
  if (argc < 2) {
    printf("USAGE: %s <id>\n", argv[0]);
    return -1;
  }
  
  char* endp = NULL;
  uint32_t gid = strtoul(argv[1], &endp, 10);
  if (*endp) {
    printf("Invalid gid %s\n", argv[1]);
    return -1;
  }
  setgid(gid);
  
  return 0;
}


static int
shell_getuid(int argc, char** argv)
{
  printf("%u\n", getuid());
  return 0;
}

static int
single_getaddrinfo(const char* a)
{
  struct addrinfo* ai = NULL;
  struct addrinfo hint = {0};
  hint.ai_family = AF_INET;
  hint.ai_flags = AI_PASSIVE;
  if (getaddrinfo(a, NULL, &hint, &ai) != 0) {
    perror("*** Addr lookup failed");
    return -1;
  }

  if (ai->ai_addr->sa_len != sizeof(struct sockaddr_in) || 
      ai->ai_addr->sa_family != AF_INET) {
    printf("*** Addr lookup failed, didn't get ipv4 addr\n");
    freeaddrinfo(ai);
    return -1;
  }

  char newip[128];
  struct sockaddr_in* si =
    (struct sockaddr_in*)ai->ai_addr;
  snprintf(newip, sizeof(newip), "%u.%u.%u.%u",
    (si->sin_addr.s_addr & 0xFF000000) >> 24,
    (si->sin_addr.s_addr & 0x00FF0000) >> 16,
    (si->sin_addr.s_addr & 0x0000FF00) >> 8,
    (si->sin_addr.s_addr & 0x000000FF));

  freeaddrinfo(ai);

  printf("%-20s %s\n", a, newip);

  return 0;
}

static int
shell_getaddrinfo(int argc, char** argv)
{
  if (argc < 2) {
    printf("USAGE: %s ip...\n", argv[0]);
    return -1;
  }

  for (int i = 1; i < argc; ++i) {
    if (single_getaddrinfo(argv[i]) < 0)
      return -1;
  }
  return 0;
}

static int
shell_apropos(int argc, char** argv)
{
  if (argc < 2) {
    printf("USAGE: %s term\n", argv[0]);
    return -1;
  }

  for (rtems_shell_cmd_t* c=rtems_shell_first_cmd; c; c=c->next) {
    if (strstr(c->name, argv[0]))
      printf("%s\n", c->name);
  }
  return 0;
}