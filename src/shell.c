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
#include <ttcp.h>

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
#include <dirent.h>

#include "rtems-init.h"
#include "common/getopt_s.h"
#include "common/util.h"

#include "config.h"

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
static int shell_rtems_sh(int argc, char** argv);

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
  { "sh",         "misc",   "",                   shell_rtems_sh },
  { "ttcp",       "net",    "ttcp test",          rtems_shell_main_ttcp },
#ifdef HAVE_LUA
  { "lua",        "misc",   LUA_USAGE,            shell_lua_main },
#endif
  { NULL,         NULL,     NULL,                 NULL },
};

#if HAVE_LUA
static int
rtems_cmd_run_lua(int argc, char** argv)
{
  if (argc < 1)
    return -1;
  const char* file = argv[0];
  if (!file)
    return -1; /* just for safety */

  char abs[PATH_MAX];
  snprintf(abs, sizeof(abs), "/bin/%s", file);

  /* TODO: might need to strdup the args; not sure if lua is doing any
   * modification of these internally. */
  char** newargs = calloc(sizeof(char*), argc+1);
  newargs[0] = "lua";
  newargs[1] = abs;
  for (int i = 2; i < argc+1; ++i) {
    newargs[i] = argv[i-1];
  }

  int r = shell_lua_main(argc+1, newargs);

  free(newargs);
  return r;
}

static void
register_lua_progs()
{
  DIR* d = opendir("/bin");
  if (!d) {
    return; /* probably doesnt exist */
  }

  struct dirent* de = NULL;
  while ((de = readdir(d)) != NULL) {
    if (strcmp(path_get_extension(de->d_name), "lua")) {
      continue;
    }

    char abs[PATH_MAX];
    snprintf(abs, sizeof(abs), "/bin/%s", de->d_name);

    /* check for valid file */
    struct stat st;
    if (stat(abs, &st) < 0)
      continue;

    /* check for exec bit. since I don't want to bother with proper perm
     * checking, we'll just require all 3 exec bits */
    const uint32_t desired = S_IXUSR | S_IXGRP | S_IXOTH;
    if ((st.st_mode & desired) != desired)
      continue;

    rtems_shell_add_cmd(
      de->d_name,
      "user-programs",
      "User provided Lua program in /bin",
      rtems_cmd_run_lua
    );
  }
  closedir(d);
}
#endif

/* Register all shell commands.
 * Also registers Lua programs in the relevant bin/ locations
 */
int
shell_register_cmds()
{
  /* register all shell commands */
  for (int i = 0;;++i) {
    struct shell_cmd cmd = shell_cmds[i];
    if (!cmd.cmd) break;
    rtems_shell_add_cmd(cmd.cmd, cmd.topic, cmd.usage, cmd.command);
  }

#if HAVE_LUA
  register_lua_progs();
#endif
  return 0;
}

int
rtems_sh()
{
  rtems_shell_env_t e;
  rtems_shell_dup_current_env(&e);

  return rtems_shell_run_main_loop(&e, true, NULL) ? 0 : 1;
}

#ifdef HAVE_CEXP
CEXP_HELP_TAB_BEGIN(rtems_sh)
	HELP(
    "Spin up an instance of the RTEMS shell\n",
	  int, rtems_sh,  (void)
	),
CEXP_HELP_TAB_END
#endif

static int
shell_rtems_sh(int argc, char** argv)
{
  return rtems_sh();
}

static int
shell_debugger_start(int argc, char** argv)
{
#ifdef HAVE_DEBUGGER
  int opt = -1, verbose = 0, timeout = RTEMS_DEBUGGER_TIMEOUT;
  char port[32] = "1234";

  struct getopt_state s = {0};
  while ((opt = getopt_s(opt, argv, "p:hv", &s)) != -1) {
    switch (opt) {
    case 'p':
      strncpySafe(port, s.optarg, sizeof(port));
      break;
    case 'v':
      verbose = 1;
      break;
    case 't':
      timeout = atoi(s.optarg);
      if (timeout <= 0) {
        fprintf(stderr, "Bad timeout value, using default\n");
        timeout = RTEMS_DEBUGGER_TIMEOUT;
      }
      break;
    default:
    case 'h':
      fprintf(stderr, "USAGE: %s [-p port] [-t timeout] [-v]\n", argv[0]);
      fprintf(stderr, "  -p PORT     : Port number to use (default: 1234)\n");
      fprintf(stderr, "  -t TIMEOUT  : Debugger timeout (default: 3)\n");
      fprintf(stderr, "  -v          : Enable verbose mode (to debug the debugger!)\n");
      return -1;
    }
  }

  rtems_debugger_register_tcp_remote();
  rtems_printer printer;
  rtems_print_printer_printf(&printer);
  rtems_debugger_set_verbose(verbose);
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
#ifdef LIBBSP_BEATNIK_BSP_H
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
  hint.ai_flags = AI_CANONNAME;
  int r;
  if ((r = getaddrinfo(a, NULL, &hint, &ai)) != 0) {
    printf("*** Addr lookup failed: %s (%d)\n", gai_strerror(r), r);
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
extern void run_tests();
extern int nfs_perf_test();

static struct bsp_test {
  const char* name;
  const char* desc;
  int (*pfn)();
} s_tests[] = {
  //{"legacy_irq", "Legacy BSP IRQ API", legacy_irq_tst},
  //{"irq", "New IRQ API", irq_tst},
  {"nfs_perf", "NFS perf test", nfs_perf_test},
};

static int
shell_test(int argc, char** argv)
{
  run_tests();
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

#if HAVE_PCI
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

struct pci_device_name
{
  uint16_t id;
  const char* name;
};

struct pci_device_name INTEL_DEVICES[] =
{
  {0x1008, "82544EI Gigabit Ethernet Controller (Copper)"},
  {0xFFFF, NULL}
};

struct pci_device_name TUNDRA_DEVICES[] =
{
  {0x0000, "CA91C042 [Universe]"},
  {0xFFFF, NULL}
};

struct pci_device_name HINT_DEVICES[] =
{
  {0x0026, "HB2 PCI-PCI Bridge"},
  {0xFFFF, NULL}
};

struct pci_device_name MARVELL_DEVICES[] =
{
  {0x6430, "MV64360 System Controller"},
  {0xFFFF, NULL}
};

static struct
{
  uint16_t vendor;
  const char* name;
  struct pci_device_name* devices;
} VENDOR_LOOKUP[] =
{
  {0x3388, "HiNT Corp.", HINT_DEVICES},
  {0x8086, "Intel Corp.", INTEL_DEVICES},
  {0x10e3, "Tundra Semiconductor Corp.", TUNDRA_DEVICES},
  {0x11ab, "Marvell Technology Group Ltd.", MARVELL_DEVICES},
};

static int
pci_describe_device(uint16_t vendor, uint16_t device, const char** vname, const char** dname)
{
  *vname = "Unknown Vendor";
  *dname = "Unknown Device";
  for (int i = 0; i < sizeof(VENDOR_LOOKUP)/sizeof(VENDOR_LOOKUP[0]); ++i) {
    if (vendor == VENDOR_LOOKUP[i].vendor) {
      *vname = VENDOR_LOOKUP[i].name;
      for (int j = 0; ; ++j) {
        if (VENDOR_LOOKUP[i].devices[j].id == 0xFFFF)
          return -1;
        if (VENDOR_LOOKUP[i].devices[j].id == device) {
          *dname = VENDOR_LOOKUP[i].devices[j].name;
          return 0;
        }
      }
    }
  }
  return -1;
}

static struct
{
  const char* name;
  uint32_t flag;
} PCI_STATUS_BITS[] =
{
  {"66MHz",       PCI_STATUS_66MHZ},
  {"UDF",         PCI_STATUS_UDF},
  {"FastB2B",     PCI_STATUS_FAST_BACK},
  {"ParErr",      PCI_STATUS_PARITY},
  {"SigTgtAbrt",  PCI_STATUS_SIG_TARGET_ABORT},
  {"RecTgtAbrt",  PCI_STATUS_REC_TARGET_ABORT},
  {"RecMstrAbrt", PCI_STATUS_REC_MASTER_ABORT},
  {"SigSysErr",   PCI_STATUS_SIG_SYSTEM_ERROR},
  {"SPerr",       PCI_STATUS_DETECTED_PARITY},
};

static int
print_pci_dev(int bus, int slot, int func, int vend, int dev)
{
  const char* typestr = "Unknown";
  int barCount = 1;
  uint8_t type = 0;
  if (pci_read_config_byte(bus, slot, func, PCI_HEADER_TYPE, &type) == 0) {
    switch (type) {
    case PCI_HEADER_TYPE_NORMAL:
      typestr = "PCI Device";
      barCount = 6;
      break;
    case PCI_HEADER_TYPE_BRIDGE:
      typestr = "PCI Bridge Device";
      barCount = 2;
      break;
    case PCI_HEADER_TYPE_CARDBUS:
      typestr = "CardBus Device"; break;
    case PCI_HEADER_TYPE_MULTI_FUNCTION:
      typestr = "Multifunction Device"; break;
    default:
      typestr = "Invalid Device Type";
    }
  }

  const char *vname, *dname;
  pci_describe_device(vend, dev, &vname, &dname);

  printf(
    "%02d:%02d.%d %s: Vendor 0x%04X Device 0x%04X: %s %s\n",
    bus, slot, func, typestr, vend, dev, vname, dname
  );

  /* Display IRQ pin */
  uint8_t irq_pin = 0, irq_line = 0;
  pci_read_config_byte(bus, slot, func, PCI_INTERRUPT_PIN, &irq_pin);
  pci_read_config_byte(bus, slot, func, PCI_INTERRUPT_LINE, &irq_line);
  printf("  IRQ %d, IRQ Pin %d\n", irq_line, irq_pin);

  /* Display class/subclass */
  uint16_t classsubclass = 0;
  pci_read_config_word(bus, slot, func, PCI_CLASS_DEVICE, &classsubclass);
  uint8_t progif = 0;
  pci_read_config_byte(bus, slot, func, PCI_REVISION_ID, &progif);
  printf(
    "  Class: 0x%02X, Subclass: 0x%02X, Prog I/F: 0x%02X\n",
    (classsubclass & 0xFF00) >> 8,
    (classsubclass & 0xFF),
    progif
  );

  /* mirrors lspci format of status bits */
  printf("  Status: ");
  uint16_t status = 0;
  if (pci_read_config_word(bus, slot, func, PCI_STATUS, &status) != 0)
    printf("<Unable to read>");
  else {
    for (int i = 0; i < sizeof(PCI_STATUS_BITS) / sizeof(PCI_STATUS_BITS[0]); ++i) {
      printf(
        "%s%s ",
        PCI_STATUS_BITS[i].name,
        (status & PCI_STATUS_BITS[i].flag) ? "+" : "-"
      );
    }
  }
  printf("\n");
  
  /* display BAR registers */
  const uint32_t bars[] = {
    PCI_BASE_ADDRESS_0,
    PCI_BASE_ADDRESS_1,
    PCI_BASE_ADDRESS_2,
    PCI_BASE_ADDRESS_3,
    PCI_BASE_ADDRESS_4,
    PCI_BASE_ADDRESS_5,
  };
  
  for (int i = 0; i < barCount; ++i) {
    printf("  BAR%d: ", i);
    uint32_t bar;
    if (0 != pci_read_config_dword(bus, slot, func, bars[i], &bar)) {
      printf("<Unable to read>\n");
      continue;
    }

    /* mem type and prefetchable flags are only valid for non-I/O spaces */
    if (bar & PCI_BASE_ADDRESS_SPACE_IO) {
      printf(
        "0x%08lX (I/O Mem, 32-bits, non-prefetchable)",
        bar & PCI_BASE_ADDRESS_IO_MASK
      );
    }
    else {
      /* display mem type */
      printf(
        "0x%08lX (",
        bar & PCI_BASE_ADDRESS_MEM_MASK
      );
      switch (bar & PCI_BASE_ADDRESS_MEM_TYPE_MASK) {
      case PCI_BASE_ADDRESS_MEM_TYPE_1M:
        printf("20-bit"); break;
      case PCI_BASE_ADDRESS_MEM_TYPE_32:
        printf("32-bit"); break;
      case PCI_BASE_ADDRESS_MEM_TYPE_64:
        printf("64-bit"); break;
      }

      /* display prefetchable or not */
      printf(
        ", %s) ",
        (bar & PCI_BASE_ADDRESS_MEM_PREFETCH) ? "prefetchable" : "non-prefetchable"
      );
    }

    /* bar size cannot be easily determined without clobbering the register, so skipping it */
    printf("\n");
  }
  
  /* type 1 options */
  if (type == PCI_HEADER_TYPE_BRIDGE) {
    uint8_t primaryBus = 0, secondaryBus = 0;
    uint8_t subBus = 0;
    pci_read_config_byte(bus, slot, func, PCI_PRIMARY_BUS, &primaryBus);
    pci_read_config_byte(bus, slot, func, PCI_SECONDARY_BUS, &secondaryBus);
    pci_read_config_byte(bus, slot, func, PCI_SUBORDINATE_BUS, &subBus);
    printf(
      "  Primary Bus: %d, Secondary Bus: %d, Subordinate: %d\n",
      primaryBus, secondaryBus, subBus
    );

    uint16_t memLimit = 0, memBase = 0;
    pci_read_config_word(bus, slot, func, PCI_MEMORY_BASE, &memBase);
    pci_read_config_word(bus, slot, func, PCI_MEMORY_LIMIT, &memLimit);

    /* aligned to 1MB boundary */
    printf(
      "  Memory Behind Bridge: 0x%08X-0x%08X\n",
      (uint32_t)(memBase) << 20,
      (uint32_t)(memLimit) << 20
    );

    uint8_t ioLimit = 0, ioBase = 0;
    uint16_t ioLimitUpper = 0, ioBaseUpper = 0;
    pci_read_config_byte(bus, slot, func, PCI_IO_BASE, &ioBase);
    pci_read_config_byte(bus, slot, func, PCI_IO_LIMIT, &ioLimit);
    pci_read_config_word(bus, slot, func, PCI_IO_BASE_UPPER16, &ioBaseUpper);
    pci_read_config_word(bus, slot, func, PCI_IO_LIMIT_UPPER16, &ioLimitUpper);

    switch (ioLimit & 0xF) {
    case 0: /* ISA compat, 16-bit addressing */
      printf(
        "  I/O Behind Bridge: 0x%04X-0x%04X (16-bit, ISA compatibility)\n",
        (uint32_t)(ioBase & 0xF) << 12,
        (uint32_t)(ioLimit & 0xF) << 12
      );
      break;
    case 1: /* 32-bit addressing */
      printf(
        "  I/O Behind Bridge: 0x%08X-0x%08X (32-bit)\n",
        (uint32_t)(ioBaseUpper) << 16 | (uint32_t)(ioBase & 0xF) << 12,
        (uint32_t)(ioLimitUpper) << 16 | (uint32_t)(ioLimit & 0xF) << 12
      );
      break;
    default:
      printf("  I/O Behind Bridge: Invalid?\n");
      /* ??? */
    }

    uint16_t prefLimit = 0, prefBase = 0;
    uint32_t prefBaseUpper, prefLimitUpper;
    pci_read_config_word(bus, slot, func, PCI_PREF_MEMORY_BASE, &prefBase);
    pci_read_config_word(bus, slot, func, PCI_PREF_MEMORY_LIMIT, &prefLimit);
    pci_read_config_dword(bus, slot, func, PCI_PREF_BASE_UPPER32, &prefBaseUpper);
    pci_read_config_dword(bus, slot, func, PCI_PREF_LIMIT_UPPER32, &prefLimitUpper);

    printf(
      "  Prefetchable Memory Behind Bridge: 0x%08X-0x%08X\n",
      (uint32_t)(prefBase) << 20,
      (uint32_t)(prefLimit) << 20
    );
  }
  
  puts("\n");
  
  return 0;
}

int lspci_adv()
{
  const uint8_t busses = pci_bus_count();
  if (busses == 0) {
    printf("No PCI busses on this system\n");
    return -1;
  }
  
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
        print_pci_dev(bus, slot, func, vendor, device);
      }
    }
  }
  return 0;
}

#ifdef HAVE_CEXP
CEXP_HELP_TAB_BEGIN(lspci)
	HELP(
    "List all PCI devices\n",
	  int, lspci,  (void)
	),
CEXP_HELP_TAB_END

CEXP_HELP_TAB_BEGIN(lspci_adv)
	HELP(
    "List all PCI devices in detail\n",
	  int, lspci_adv,  (void)
	),
CEXP_HELP_TAB_END
#endif // HAVE_CEXP

#endif // HAVE_PCI

static int
shell_pci_probe(int argc, char** argv)
{
#if HAVE_PCI
  bool advanced = false;
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-v"))
      advanced = true;
    else {
      printf("Unknown arg %s\n", argv[i]);
      return -1;
    }
  }
  return advanced ? lspci_adv() : lspci();
#else
  printf("PCI not supported by this BSP\n");
  return -1;
#endif
}


