#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>

#include <bsp.h>
#include <rtems.h>
#include <rtems/rtems-debugger.h>
#include <rtems/rtems-debugger-remote-tcp.h>
#include <rtems/shell.h>
#include <rtems/bsd.h>
#include <rtems/bsd/bsd.h>
#include <rtems/dhcpcd.h>
#include <rtems/bsd/iface.h>
#include <rtems/bsd/modules.h>
#include <rtems/ntpd.h>
#include <rtems/ntpq.h>
#include <machine/rtems-bsd-commands.h>
#include <machine/rtems-bsd-rc-conf-services.h>
#include <rtems/bspcmdline.h>
#include <rtems/rtl/dlfcn-shell.h>
#include <rtems/rtl/rtl.h>
#include <rtems/rtl/rtl-shell.h>
#include <rtems/telnetd.h>
#include <rtems/imfs.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>

#include "rtems-init.h"
#include "getopt_s.h"
#include "util.h"

static const char* BANNER =
"  ____  _           _      _____      _____   _______  _____  __     __   ____ \n"
" / __ \\| |         / \\    / ___ \\ |  |  __ \\ |___ ___||  ___||  \\   /  | / __ \\  \n"
"| (  \\/| |        /   \\  / /   \\/ |  | |__| |   | |   | |___ |   \\ /   || (__\\/  \n"
" ============================O    |  |  _  /    | |   |  ___|| |\\ V /| | \\__ \\ \n"
"/\\__) || |____  / _____ \\\\ \\___/\\ |  | | \\ \\    | |   | |___ | | \\ / | |/\\__) |  \n"
"\\____/ |______|/_/     \\\\_\\_____/ |  |_|  \\_\\   |_|   |_____||_|  V  |_|\\____/ \n"
" National Accelerator Laboratory  | Real Time Executive for Multiprocessor Systems \n"
"---------------------------> SLAC RTEMS Distribution <---------------------------- \n";

static void telnetd_init_command(char*, void*);

struct dhcp_runtime_cfg dhcp_runtime_cfg;

int verbose = 1;

enum init_mode init_mode = INIT_MODE_CMDLINE;

rtems_telnetd_config_table rtems_telnetd_config = {
  .stack_size = 0,
  .login_check = NULL,
  .client_maximum = 0,
  .port = 5512,
  .keep_stdio = 0,
  .command = telnetd_init_command,
};

static void
bsp_cmdline_get_param(const char* param, char* val, size_t vlen)
{
  rtems_bsp_cmdline_get_param(param, val, vlen);
}

static int
bsp_cmdline_has_param(const char* param)
{
  return NULL != rtems_bsp_cmdline_get_param_raw(param);
}

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

/**
 * Do serial init tasks, mostly to disable certain types of control on stdin
 */
void
serial_init()
{
  struct termios tio;
  if (tcgetattr(fileno(stdin), &tio) < 0) {
    perror("tcgetattr failed");
  }
  
  tio.c_iflag &= (IXOFF|IXON|IXANY|IGNBRK);
  tio.c_iflag |= BRKINT;
  if (tcsetattr(fileno(stdin), TCSANOW, &tio) < 0) {
    perror("tcsetattr failed");
  }
  
  /** Display our really cool banner */
  puts(BANNER);

  printf("*** RTEMS : %s\n", rtems_get_version_string());
  printf("*** SLAC RTEMS Init System : Built %s %s\n", __DATE__, __TIME__);
  printf("*** BSP command line: %s\n", rtems_bsp_cmdline_get());

#ifdef BSP_I2C_BUS0_NAME
  BSP_i2c_initialize();
#endif
}

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

static void
generate_resolv_conf()
{
  FILE* fp = fopen("/etc/resolv.conf", "wb");
  if (!fp) {
    printf("*** Failed to create /etc/resolv.conf\n");
    return;
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
}

/**
 * Init network and libbsd
 */
void
network_init()
{
  int r;
  printf("** Begin network init\n");

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

  printf("** End network init\n");
}

static int
do_mount(const char* ip, const char* src, const char* mntpt, 
  uint32_t uid, uint32_t gid, enum fstype type)
{
  const char* fs;
  int vers = 2;
  switch (type) {
  case FS_TYPE_NFS3:
    vers = 3, fs = RTEMS_FILESYSTEM_TYPE_NFS;
    break;
  case FS_TYPE_NFS4:
    vers = 4, fs = RTEMS_FILESYSTEM_TYPE_NFS;
    break;
  case FS_TYPE_NFS2:
    vers = 2, fs = RTEMS_FILESYSTEM_TYPE_NFS;
    break;
  case FS_TYPE_9P:
    /** Unsupported for now */
  default:
    printf("*** Unsupported FS type\n");
    return -1;
  }

  /** Ensure mount point exists */
  rtems_mkdir(mntpt, 0777);

  struct addrinfo* ai = NULL;
  struct addrinfo hint = {0};
  hint.ai_family = AF_INET;
  hint.ai_flags = AI_PASSIVE;
  if (getaddrinfo(ip, NULL, &hint, &ai) != 0) {
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

  char source[512];
  snprintf(source, sizeof(source), "%s:%s", newip, src);

  char opts[512] = {0};
  /** FIXME: uid gid? */
  /** Always mounting these as read-only, data area should be rw  */
  snprintf(opts, sizeof(opts), "vers=%d,ro", vers);

  if (mount(source, mntpt, fs, 0, opts) < 0)
    return -1;

  printf("*** Mounted %s:%s at %s\n", ip, src, mntpt);
  return 0;
}

/**
 * Init mounts as specified by DHCP or NVRAM
 */
void
mounts_init()
{
  char ip[MNT_STR_BUF_SZ], src[MNT_STR_BUF_SZ], mntpt[MNT_STR_BUF_SZ];
  char file[MNT_STR_BUF_SZ];
  uint32_t gid = 0, uid = 0;
  enum fstype fs;
  const char* bp = NULL;
  char value[512];
  int opt = 0;

  printf("** Setting up mounts\n");

  /** Mount FS that includes the boot file */
  bp = getenv("BP_FILE");
  if (bp) {
    if (parse_mount_spec(bp, &fs, &uid, &gid, ip, src, mntpt, file) < 0) {
      printf("*** BP_FILE malformed, unable to parse\n");
    }
    
    if (do_mount(ip, src, mntpt, uid, gid, fs) < 0) {
      printf("*** Mount failed for %s:%s:%s\n", ip, src, mntpt);
    }
  }

  /** Mount FS that includes cmdline */
  bp = getenv("BP_PARM");
  if (bp) {
    /** FIXME: actually parse this lol */
    if (!strncmp(bp, "INIT=", sizeof("INIT=")-1))
      bp += sizeof("INIT=")-1;
    
    if (parse_mount_spec(bp, &fs, &uid, &gid, ip, src, mntpt, file) < 0) {
      printf("*** BP_PARM malformed, unable to parse\n");
      goto cmdline_mnt;
    }

    if (ismounted(mntpt)) {
      printf("*** %s already mounted, skipping\n", mntpt);
      goto cmdline_mnt;
    }

    if (do_mount(ip, src, mntpt, uid, gid, fs) < 0) {
      printf("*** Mount failed for %s:%s:%s\n", ip, src, mntpt);
    }
  }
  else {
    printf("*** No BP_PARM. Missing from NVRAM and DHCP?\n");
  }

cmdline_mnt:

  

#if __i386__
  /** On i386, parse mounts provided by BSP command line */
  if (rtems_bsp_cmdline_get_param("--mount", value, sizeof(value))) {
    if (parse_mount_spec(value + sizeof("--mount"), 
        &fs, &uid, &gid, ip, src, mntpt, file) < 0) {
      printf("*** Unable to parse --mount\n");
      goto end;
    }

    if (ismounted(file)) {
      printf("*** %s already mounted, skipping\n", file);
      goto end;
    }

    if (do_mount(ip, src, file, uid, gid, fs) < 0) {
      printf("*** Mount failed for %s:%s:%s\n", ip, src, file);
    }
  }
#endif

end:
  return;
}

/**
 * Initialize in-memory FS basics, must be done before shell and dhcpd init
 */
void
imfs_init()
{
  printf("Unpacking rootfs...\n");
  /** Unpack the rootfs */
  setuid(0);
  //unpack_rootfs();

  rtems_tarfs_load("/", tar_rootfs, tar_rootfs_SIZE);
}

/**
 * Initialize RTEMS shell
 */
void
shell_init()
{
  printf("** Begin shell init\n");
  rtems_shell_init_environment();

  char val[256];
  const char* nd = rtems_bsp_cmdline_get_param("--cwd", val, sizeof(val));
  if (nd) {
    strcpy(rtems_shell_get_current_env()->cwd, nd + sizeof("--cwd"));
  }

  for (int i = 0;;++i) {
    struct shell_cmd cmd = shell_cmds[i];
    if (!cmd.cmd) break;
    rtems_shell_add_cmd(cmd.cmd, cmd.topic, cmd.usage, cmd.command);
  }

  rtems_status_code r;
  r = rtems_shell_init(
    "SHLL", 0, 100, "/dev/console", true, false, NULL
  );

  if (r != RTEMS_SUCCESSFUL)
    printf("Unable to init RTEMS shell\n");
  
  printf("** End shell init\n");

  rtems_termios_register_isig_handler(rtems_termios_posix_isig_handler);
}

/**
 * Main RTEMS entry point
 */
void*
POSIX_Init(void *argument)
{
  nvram_init();
  serial_init();
  imfs_init();
  network_init();
  mounts_init();
  shell_init();
  return 0;
}

/* Ensure that stdio goes to serial (so it can be captured) */
#if defined(__i386__) && !USE_COM1_AS_CONSOLE
#include <uart.h>
#if __RTEMS_MAJOR__ > 4
#include <libchip/serial.h>
#endif

extern int BSPPrintkPort;
void 
bsp_predriver_hook(void)
{
#if __RTEMS_MAJOR__ > 4
    Console_Port_Minor = BSP_CONSOLE_PORT_COM1;
#else
    BSPConsolePort = BSP_CONSOLE_PORT_COM1;

#endif
    BSPPrintkPort = BSP_CONSOLE_PORT_COM1;
}
#endif
