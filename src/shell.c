/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Misc shell utilities for the RTEMS shell and Cexpsh
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
#include <rtems/shell.h>
#include <rtems/rtl/dlfcn-shell.h>
#include <rtems/rtl/rtl-shell.h>
#include <rtems/ntpq.h>
#include <rtems/ntpd.h>
#include <bsp.h>
#include <rtems/libi2c.h>

#ifdef RTEMS_BSD_STACK
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#endif

#include <rtems/pci.h>

#ifdef HAVE_DEBUGGER
#include <rtems/rtems-debugger.h>
#include <rtems/rtems-debugger-remote-tcp.h>
#endif

#ifdef HAVE_CEXP
#include <cexpHelp.h>
#else
#define CEXP_HELP_TAB_BEGIN(...)
#define HELP(...)
#endif

#include <dlfcn.h>
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
static int shell_test(int argc, char** argv);
static int shell_sysReset(int argc, char** argv);
static int shell_getifaddrs(int argc, char** argv);
static int shell_pci_probe(int argc, char** argv);
static int shell_sh(int argc, char** argv);

extern int shell_lua_main(int argc, char** argv);

#define GEV_SHOW_USAGE "gevShow -- Display all GEV NVRAM parameters"
#define GEV_GET_USAGE "gevGet paramName -- Display value of a specific GEV parameter"
#define NVRAM_SHOW_USAGE "nvramShow -- Display all NVRAM boot parameters"
#define NVRAM_GET_USAGE "nvramGet paramName -- Display value of specific NVRAM boot parameter"
#define APROPOS_USAGE "apropos term -- Search command list 'term'"
#define SETUID_USAGE "setuid UID -- Sets the current effective UID"
#define GETUID_USAGE "getuid -- Gets the current effective UID"
#define SETGID_USAGE "setgid -- Sets the current effective GID"
#define GETADDRINFO_USAGE "getaddrinfo loc -- Perform a dns lookup on 'loc'"
#define DUMPENV_USAGE "dumpenv -- Dump the environment"
#define LSPCI_USAGE "lspci -- Probe PCI buses"
#define LUA_USAGE "lua -- Start interactive lua interpreter"

/** !!! This is internal and I probably shouldn't do this! */
extern rtems_shell_cmd_t* rtems_shell_first_cmd;

struct shell_cmd shell_cmds[] =
{
  /* cmd, topic, usage, func */
  { "dlopen",     "rtl",    "",                   shell_dlopen },
  { "dlsym",      "rtl",    "",                   shell_dlsym },
  { "dlclose",    "rtl",    "",                   shell_dlclose },
  { "dlcall",     "rtl",    "",                   shell_dlcall },
  { "rtl",        "rtl",    "",                   rtems_rtl_shell_command },
  { "dbgstart",   "misc",   "",                   shell_debugger_start },
  { "dbgstop",    "misc",   "",                   shell_debugger_stop },
  { "nvramGet",   "nvram",  NVRAM_GET_USAGE,      shell_nvram_get },
  { "nvramShow",  "nvram",  NVRAM_SHOW_USAGE,     shell_nvram_show },
  { "gevGet",     "nvram",  GEV_GET_USAGE,        shell_gev_get },
  { "gevShow",    "nvram",  GEV_SHOW_USAGE,       shell_gev_show },
  { "temp",       "misc",   "",                   shell_read_temp },
  { "ntpd",       "net",    "",                   rtems_ntpd_run },
  { "dumpenv",    "misc",   DUMPENV_USAGE,        shell_dumpenv },
  { "setuid",     "misc",   SETUID_USAGE,         shell_setuid },
  { "getuid",     "misc",   GETUID_USAGE,         shell_getuid },
  { "setgid",     "misc",   SETGID_USAGE,         shell_setgid },
  { "getaddrinfo","net",    GETADDRINFO_USAGE,    shell_getaddrinfo },
  { "apropos",    "misc",   APROPOS_USAGE,        shell_apropos },
  { "test",       "misc",   "",                   shell_test },
  { "getifaddrs", "net",    "",                   shell_getifaddrs },
  { "lspci",      "misc",   "",                   shell_pci_probe },
  { "sh",         "misc",   "",                   shell_sh },
#ifdef HAVE_LUA
  { "lua",        "misc",   LUA_USAGE,            shell_lua_main },
#endif
  { NULL,         NULL,     NULL,                 NULL },
};

static int
shell_debugger_start(int argc, char** argv)
{
#ifdef HAVE_DEBUGGER
  int opt = -1;
  char port[32] = "1234";

  struct getopt_state s = {0};
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
#else
  fprintf(stderr, "Failed to start debugger: unsupported BSP\n");
  return -1;
#endif
}

static int
shell_debugger_stop(int argc, char** argv)
{
#ifdef HAVE_DEBUGGER
  if (!rtems_debugger_running()) {
    fprintf(stderr, "debugger is not running\n");
    return -1;
  }
  rtems_debugger_stop();
  return 0;
#else
  return -1;
#endif
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
  si->sin_addr.s_addr = ntohl(si->sin_addr.s_addr);
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
    if (strstr(c->name, argv[1]) || strstr(c->usage, argv[1]))
      printf("%s\n", c->name);
  }
  return 0;
}

/** tests **/
extern int legacy_irq_tst();
extern int irq_tst();
extern int nfs_perf_test();

static struct bsp_test {
  const char* name;
  const char* desc;
  int (*pfn)();
} s_tests[] = {
  {"legacy_irq", "Legacy BSP IRQ API", legacy_irq_tst},
  {"irq", "New IRQ API", irq_tst},
  {"nfs_perf", "NFS perf test", nfs_perf_test},
};

static int
shell_test(int argc, char** argv)
{
  const char* filter = argc > 1 ? argv[1] : NULL;
  puts(ANSI_BLUE "===== BSP Test Suite =====" ANSI_RESET);
  for (int i = 0; i < sizeof(s_tests)/sizeof(*s_tests); ++i) {
    if (filter && !strstr(s_tests[i].name, filter))
      continue;

    printf("--> Running %s\n", s_tests[i].desc);
    if (s_tests[i].pfn() != 0)
      puts(ANSI_RED "  *** Failed" ANSI_RESET);
    else
      puts(ANSI_GREEN "  *** Passed" ANSI_RESET);
  }
  return 0;
}

static int
shell_sysReset(int argc, char** argv)
{
  return 0;
}

#ifdef RTEMS_BSD_STACK
static struct {
  int family;
  const char* label;
} AddressFamilies[] = {
  {AF_INET, "AF_INET"},
  {AF_INET6, "AF_INET6"},
  {AF_LINK, "AF_LINK"},
};

static const char*
_get_addr_family(int fam)
{
  for (int i = 0; i < sizeof(AddressFamilies)/sizeof(*AddressFamilies); ++i) {
    if (fam == AddressFamilies[i].family)
      return AddressFamilies[i].label;
  }
  return "Unknown";
}

static const char*
_ntoa(struct sockaddr* sa)
{
  return inet_ntoa(((struct sockaddr_in*)sa)->sin_addr);
}

static struct {
  uint32_t flag;
  const char* label;
} FlagMappings[] = {
  {IFF_UP,            "UP"},
  {IFF_UP,            "IFF_UP"},
  {IFF_BROADCAST,     "IFF_BROADCAST"},
  {IFF_DEBUG,         "IFF_DEBUG"},
  {IFF_LOOPBACK,      "IFF_LOOPBACK"},
  {IFF_POINTOPOINT,   "IFF_POINTOPOINT"},
  {IFF_DRV_RUNNING,   "IFF_DRV_RUNNING"},
  {IFF_NOARP,         "IFF_NOARP"},
  {IFF_PROMISC,       "IFF_PROMISC"},
  {IFF_ALLMULTI,      "IFF_ALLMULTI"},
  {IFF_DRV_OACTIVE,   "IFF_DRV_OACTIVE"},
  {IFF_SIMPLEX,       "IFF_SIMPLEX"},
  {IFF_LINK0,         "IFF_LINK0"},
  {IFF_LINK1,         "IFF_LINK1"},
  {IFF_LINK2,         "IFF_LINK2"},
  {IFF_ALTPHYS,       "IFF_ALTPHYS"},
  {IFF_MULTICAST,     "IFF_MULTICAST"},
  {IFF_CANTCONFIG,    "IFF_CANTCONFIG"},
  {IFF_PPROMISC,      "IFF_PPROMISC"},
  {IFF_MONITOR,       "IFF_MONITOR"},
  {IFF_STATICARP,     "IFF_STATICARP"},
  {IFF_STICKYARP,     "IFF_STICKYARP"},
  {IFF_DYING,         "IFF_DYING"},
  {IFF_RENAMING,      "IFF_RENAMING"},
};
#endif

static int
shell_getifaddrs(int argc, char** argv)
{
#ifdef RTEMS_BSD_STACK
  struct ifaddrs *ifa = NULL;
  if (getifaddrs(&ifa) < 0) {
    perror("getifaddrs");
    return -1;
  }

  for (struct ifaddrs* f = ifa; f; f = f->ifa_next) {
    printf("%s\n", f->ifa_name);
    printf("  family: %s (%d)\n", _get_addr_family(f->ifa_addr->sa_family),
      f->ifa_addr->sa_family);
    printf("  flags: ");
    for (int i = 0; i < sizeof(FlagMappings)/sizeof(*FlagMappings); ++i) {
      if (f->ifa_flags & FlagMappings[i].flag)
        printf("%s ", FlagMappings[i].label);
    }
    printf("\n");
    if (f->ifa_addr->sa_family == AF_INET)
      printf("  inet_addr: %s\n", _ntoa(f->ifa_addr));
    if (f->ifa_dstaddr->sa_family == AF_INET)
      printf("  inet_dstaddr: %s\n", _ntoa(f->ifa_dstaddr));
    if (f->ifa_netmask->sa_family == AF_INET)
      printf("  inet_netmask: %s\n", _ntoa(f->ifa_netmask));
    
    if (f->ifa_addr->sa_family == AF_LINK) {
      struct sockaddr_dl* dl = (struct sockaddr_dl*)f->ifa_addr;
      printf("  sdl_index: %d\n", dl->sdl_index);
      printf("  sdl_type: %d\n", dl->sdl_type);
    }
  }

  freeifaddrs(ifa);
#else
  printf("Unsupported on the legacy stack\n");
#endif
  return 0;
}

int
lspci()
{
  const uint8_t busses = pci_bus_count();
  if (busses == 0) {
    printf("No PCI busses on this system\n");
    return -1;
  }

  printf("%-6s %-6s %-8s %6s:%-8s %-4s\n", "BUS", "SLOT", "FUNC", "VENDOR", "DEVICE", "TYPE");
  for (int bus = 0; bus < busses; ++bus) {
    for (int slot = 0; slot < PCI_MAX_DEVICES; ++slot) {
      for (int func = 0; func < PCI_MAX_FUNCTIONS; ++func) {
        uint16_t vendor, device;
        if (pci_read_config_word(bus, slot, func, PCI_VENDOR_ID, &vendor) != 0)
          continue;
        if (pci_read_config_word(bus, slot, func, PCI_DEVICE_ID, &device) != 0)
          continue;
        if (vendor == 0xFFFF && device == 0xFFFF)
          continue;
        
        uint8_t type = 0;
        if (pci_read_config_byte(bus, slot, func, PCI_HEADER_TYPE, &type) != 0)
          printf("failed to read PCI_HEADER_TYPE\n");
          
        printf("0x%04x 0x%04x 0x%04x   0x%04x:0x%04x   0x%04x\n",
          bus, slot, func, vendor, device, (int)type);
      }
    }
  }
  
  return 0;
}

CEXP_HELP_TAB_BEGIN(lspci)
	HELP(
    "List all PCI devices\n",
	  int, lspci,  (void)
	),
CEXP_HELP_TAB_END

static int
shell_pci_probe(int argc, char** argv)
{
  return lspci();
}

int
sh()
{
  return rtems_shell_main_loop(rtems_shell_get_current_env());
}

CEXP_HELP_TAB_BEGIN(sh)
	HELP(
    "Run an instance of the RTEMS shell\n",
	  int, sh,  (void)
	),
CEXP_HELP_TAB_END

static int
shell_sh(int argc, char** argv)
{
  return sh();
}
