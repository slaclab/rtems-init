/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Core init code
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
#include <bsp.h>
#include <rtems.h>
#include <rtems/shell.h>
#include <rtems/bspcmdline.h>
#include <rtems/rtl/dlfcn-shell.h>
#include <rtems/rtl/rtl.h>
#include <rtems/rtl/rtl-shell.h>
#include <rtems/imfs.h>
#include <rtems/sysinit.h>
#include <rtems/rtems-fdt.h>
#include <bsp/fdt.h>

#if HAVE_PCI
#include <bsp/pci.h>
#endif

#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>

#include "config.h"
#include "rtems-init-config.h"

#ifdef HAVE_LUA
#include "lua.h"
#endif

#ifdef HAVE_CEXP
#include "cexp.h"
#include "cexpmod.h"
#endif

#include "rtems-init.h"
#include "getopt_s.h"
#include "util.h"

extern void run_tests();

static const char* BANNER =
"\e[31m  ____  _           _      _____      _____   _______  _____  __     __   ____ \e[0m\n"
"\e[31m / __ \\| |         / \\    / ___ \\ |  |  __ \\ |___ ___||  ___||  \\   /  | / __ \\  \e[0m\n"
"\e[31m| (  \\/| |        /   \\  / /   \\/ |  | |__| |   | |   | |___ |   \\ /   || (__\\/  \e[0m\n"
"\e[31m ============================O    |  |  _  /    | |   |  ___|| |\\ V /| | \\__ \\ \e[0m\n"
"\e[31m/\\__) || |____  / _____ \\\\ \\___/\\ |  | | \\ \\    | |   | |___ | | \\ / | |/\\__) |  \e[0m\n"
"\e[31m\\____/ |______|/_/     \\\\_\\_____/ |  |_|  \\_\\   |_|   |_____||_|  V  |_|\\____/ \e[0m\n"
"\e[31m National Accelerator Laboratory  | Real Time Executive for Multiprocessor Systems \e[0m\n";

struct dhcp_runtime_cfg dhcp_runtime_cfg;

int verbose = 1;

enum init_mode init_mode = INIT_MODE_CMDLINE;

char startup_script[PATH_MAX];

/**
 * Do serial init tasks, mostly to disable certain types of control on stdin
 */
void
serial_init()
{
  struct termios tio;
  if (tcgetattr(fileno(stdin), &tio) < 0) {
    kerror("tcgetattr: %s\n", strerror(errno));
  }
  
  tio.c_iflag &= (IXOFF|IXON|IXANY|IGNBRK);
  tio.c_iflag |= BRKINT;
  tio.c_lflag |= ISIG;
  if (tcsetattr(fileno(stdin), TCSANOW, &tio) < 0) {
    kerror("tcsetattr: %s\n", strerror(errno));
  }
  
  /** Display our really cool banner */
  puts(BANNER);

  kclog(KLOG_BLUE, "*** RTEMS : %s\n", rtems_get_version_string());
  kclog(KLOG_BLUE, "*** SLAC RTEMS Init System : Built %s %s\n", __DATE__, __TIME__);
#ifdef RTEMS_BSD_STACK
  kclog(KLOG_BLUE, "*** Network Stack : BSD\n");
#elif defined(RTEMS_LEGACY_STACK)
  kclog(KLOG_BLUE, "*** Network Stack : Legacy\n");
#else
  kclog(KLOG_BLUE, "*** No networking stack\n");
#endif
  klog("BSP command line: %s\n", rtems_bsp_cmdline_get());

#ifdef BSP_I2C_BUS0_NAME
  BSP_i2c_initialize();
#endif
}

static int
do_mount(const char* ip, const char* src, const char* mntpt, 
  uint32_t uid, uint32_t gid, enum fstype type)
{
  const char* fs = NULL;
  int vers = 3, isnfs = 0;
  switch (type) {
  case FS_TYPE_NFS3:
    vers = 3, fs = RTEMS_FILESYSTEM_TYPE_NFS;
    isnfs = 1;
    break;
  case FS_TYPE_NFS4:
    vers = 4, fs = RTEMS_FILESYSTEM_TYPE_NFS;
    isnfs = 1;
    break;
  case FS_TYPE_NFS2:
    vers = 2, fs = RTEMS_FILESYSTEM_TYPE_NFS;
    isnfs = 1;
    break;
  case FS_TYPE_9P:
    /** Unsupported for now */
  default:
    kerror("do_mount: unsupported FS type: %d\n", type);
    return -1;
  }

  /** Ensure mount point exists */
  rtems_mkdir(mntpt, 0777);

  struct addrinfo* ai = NULL;
  struct addrinfo hint = {0};
  hint.ai_family = AF_INET;
  hint.ai_flags = AI_PASSIVE;
  if (getaddrinfo(ip, NULL, &hint, &ai) != 0) {
    kerror("do_mount: addr lookup failed: %s\n", strerror(errno));
    return -1;
  }

  if (ai->ai_addr->sa_len != sizeof(struct sockaddr_in) || 
      ai->ai_addr->sa_family != AF_INET) {
    kerror("do_mount: addr lookup failed, didn't get ipv4 addr\n");
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
  /* FIXME: uid gid? */
  /* Always mounting these as read-only, data area should be rw  */
  if (isnfs)
    snprintf(opts, sizeof(opts), "vers=%d,ro", vers);

  if (mount(source, mntpt, fs, 0, opts) < 0)
    return -1;

  klog("Mounted %s:%s at %s\n", ip, src, mntpt);
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

  klog("Setting up mounts\n");

  /* Mount FS that includes the boot file */
  bp = getenv("BP_FILE");
  if (bp) {
    if (parse_mount_spec(bp, &fs, &uid, &gid, ip, src, mntpt, file) < 0) {
      kerror("BP_FILE malformed, unable to parse\n");
    }

    if (do_mount(ip, src, mntpt, uid, gid, fs) < 0) {
      kerror("Mount failed for %s:%s:%s\n", ip, src, mntpt);
    }
  }

  /** Mount FS that includes cmdline */
  bp = getenv("BP_PARM");
  if (bp) {
    /** FIXME: actually parse this lol */
    if (!strncmp(bp, "INIT=", sizeof("INIT=")-1))
      bp += sizeof("INIT=")-1;

    if (parse_mount_spec(bp, &fs, &uid, &gid, ip, src, mntpt, file) < 0) {
      kerror("BP_PARM malformed, unable to parse\n");
      goto cmdline_mnt;
    }

    /* Build startup script name */
    snprintf(startup_script, sizeof(startup_script), "%s/%s", mntpt, file);

    if (ismounted(mntpt)) {
      kwarn("%s already mounted, skipping\n", mntpt);
      goto cmdline_mnt;
    }

    if (do_mount(ip, src, mntpt, uid, gid, fs) < 0) {
      kerror("Mount failed for %s:%s:%s\n", ip, src, mntpt);
    }
  }
  else {
    kwarn("No BP_PARM. Missing from NVRAM and DHCP?\n");
  }

cmdline_mnt:

#if __i386__
  /** On i386, parse mounts provided by BSP command line */
  if (rtems_bsp_cmdline_get_param("--mount", value, sizeof(value))) {
    if (parse_mount_spec(value + sizeof("--mount"), 
        &fs, &uid, &gid, ip, src, mntpt, file) < 0) {
      kerror("Unable to parse --mount\n");
      goto end;
    }

    if (ismounted(file)) {
      kwarn("%s already mounted, skipping\n", file);
      goto end;
    }

    if (do_mount(ip, src, file, uid, gid, fs) < 0) {
      kerror("Mount failed for %s:%s:%s\n", ip, src, file);
    }
  }
#endif

end:
  return;
}

void
path_init()
{
  /* dump all bootp/dhcp settings*/
  klog("BOOTP Setting Summary:\n");
  for (const char** s = bootp_params; *s; s++) {
    const char* e = getenv(*s);
    klog("%s=%s\n", *s, e ? e : "");
  }

  const char* bpf = getenv("BP_FILE");
  if (!bpf)
    return;

  const char* path = getenv("PATH");

  enum fstype fstype;
  uint32_t uid, gid;
  char ip[MNT_STR_BUF_SZ], src[MNT_STR_BUF_SZ];
  char mntpt[MNT_STR_BUF_SZ], file[MNT_STR_BUF_SZ];

  /* parse the BP_FILE again */
  if (parse_mount_spec(bpf, &fstype, &uid, &gid, ip, src, mntpt, file) < 0) {
    kerror("BP_FILE malformed, unable to parse\n");
    return;
  }

  strip_filename(file);

  char pathbuf[1024];
  snprintf(
    pathbuf,
    sizeof(pathbuf),
    "%s%s%s/%s",
    path ? path : "",
    path ? ":" : "",
    mntpt,
    file
  );

  setenv("PATH", pathbuf, 1);
}

/**
 * Initialize in-memory FS basics, must be done before shell and dhcpd init
 */
void
imfs_init()
{
  klog("Unpacking rootfs...\n");
  /** Unpack the rootfs */
  setuid(0);
  if (rtems_tarfs_load("/", tar_rootfs, tar_rootfs_SIZE) < 0) {
    kerror("unable to unpack rootfs!\n");
    rtems_panic("unable to unpack rootfs!\n");
  }
  klog("Finished unpacking rootfs\n");
}

/**
 * Initialize RTEMS shell
 */
void
shell_init(bool early)
{
  klog("Starting interactive shell\n");

  rtems_shell_init_environment();

  char val[256];
  const char* nd = rtems_bsp_cmdline_get_param("--cwd", val, sizeof(val));
  if (nd) {
    strcpy(rtems_shell_get_current_env()->cwd, nd + sizeof("--cwd"));
  }

  /* register all shell commands */
  for (int i = 0;;++i) {
    struct shell_cmd cmd = shell_cmds[i];
    if (!cmd.cmd) break;
    rtems_shell_add_cmd(cmd.cmd, cmd.topic, cmd.usage, cmd.command);
  }

  rti_shell_type_t which = RTI_CONFIG_LOGIN_SHELL;

  /* temp hack for mvme5500 */
#ifdef BSP_beatnik
  if (BSP_getBoardType() == MVME5500)
    which = RTI_SH_RTSH;
#endif

  /* early shell will always use rtems shell */
  if (early)
    which = RTI_SH_RTSH;

  if (which == RTI_SH_CEXP) {
#ifdef HAVE_CEXP
    /* Set prompt to correspond to IOC name */
    char prompt[128];
    const char* name = getenv("BSP_MYNM");
    snprintf(prompt, sizeof(prompt), "%s>", name ? name : bsp_get_name());
    cexpSetPrompt(CEXP_PROMPT_GBL, prompt);

    /* start interactive shell */
    while (1) {
      cexpsh(NULL);
    }
#endif
  }
  else if (RTI_SH_RTSH) {
    rtems_status_code r;
    r = rtems_shell_init(
      "SHLL", 0, 100, "/dev/console", true, early, NULL
    );

    if (r != RTEMS_SUCCESSFUL)
      kerror("Unable to init RTEMS shell\n");

    klog("Finished interactive shell init\n");

    rtems_termios_register_isig_handler(rtems_termios_posix_isig_handler);
  }
}

void
earlyshell_prompt()
{
#if RTI_CONFIG_EARLYSHELL_TIMEOUT != 0
  /* Setup input to get keypresses immediately */
  struct termios old;
  if (ios_immediate_input(STDIN_FILENO, &old) < 0) {
    perror("ios_immediate_input");
    return;
  }

  ssize_t timeo = RTI_CONFIG_EARLYSHELL_TIMEOUT * 10;
  int c;
  while ((c = getchar()) <= 0 && timeo > 0) {
    usleep(100000);
    if ((timeo % 10) == 0) {
      printk(
        "Press any key to skip initialization: %lld    \r",
        (longlong_t)timeo / 10
      );
    }
    timeo--;
  }
  printk("\n");

  ios_restore(STDIN_FILENO, &old);

  if (c < 0 || timeo <= 0)
    return;

  printk("Starting early debug shell\n");
  printk("On exit, the system will continue with initialization\n");
  shell_init(true);
#endif
}

/**
 * Early system init tasks (before drivers, the rest of the system)
 */
void
early_init()
{
#ifdef BSP_uC5282
  /* hack to workaround missing SYS_CLOCK_SPEED. seems like autodetection
   * of clock speed fails more often than not. Just assume 64MHz for now */
  if (!bsp_getbenv("SYS_CLOCK_SPEED")) {
    printk("SYS_CLOCK_SPEED not set in bootloader env, defaulting to 64MHz\n");
    extern uint32_t BSP_sys_clk_speed;
    BSP_sys_clk_speed = 64000000;
  }
#elif defined(BSP_mvme3100)
  //printf("** Setting fdt\n");
  //bsp_fdt_copy(system_dtb);
#endif
}

/**
 * Execute the rc.lua file
 */
static void
rc_init()
{
#ifdef HAVE_CEXP
  /* Exec cexpsh rc script */
  if (file_exists("/etc/rc.cmd")) {
    klog("Running /etc/rc.cmd\n");
    cexpsh("/etc/rc.cmd");
  }
#endif

  /* Exec lua rc script */
  if (file_exists("/etc/rc.lua")) {
    klog("Running /etc/rc.lua\n");
    lua_exec_script("/etc/rc.lua");
  }
}

static int
initd_dirent_filter(const struct dirent* de)
{
  if (!S_ISREG(de->d_type))
    return 0;
  if (script_get_type(de->d_name) == SCRIPT_UNKNOWN)
    return 0;
  return 1;
}

int
cexpsh_exec_script(const char* script)
{
#ifdef HAVE_CEXP
  char dir[PATH_MAX];
  strncpySafe(dir, script, sizeof(dir));
  strip_filename(dir);

  char olddir[PATH_MAX];
  getcwd(olddir, sizeof(olddir));

  if (chdir(dir) < 0) {
    kwarn("chdir to %s failed: %s\n", dir, strerror(errno));
    return -1;
  }

  /* TODO: Is this really needed? */
  char path[PATH_MAX];
  strncpySafe(path, script, sizeof(path));

  klog("Running %s\n", script);
  int r = cexpsh(path);

  chdir(olddir);
  return r;
#else
  klog("Cannot execute '%s': Cexpsh support not compiled in\n", script);
  return -1;
#endif
}

/**
 * Execute any scripts in /etc/init.d
 */
static void
initd_init()
{
  int n, r;
  struct dirent** dirs = NULL;
  n = scandir(
    "/etc/init.d",
    &dirs,
    initd_dirent_filter,
    alphasort
  );

  if (n < 0)
    return;

  for (int i = 0; i < n; ++i) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/etc/init.d/%s", dirs[i]->d_name);
    klog("Running %s\n", path);

    enum script_type type = script_get_type(dirs[i]->d_name);
    switch (type) {
    case SCRIPT_CEXPSH:
      r = cexpsh_exec_script(path);
      break;
    case SCRIPT_LUA:
      r = lua_exec_script(path);
      break;
    default:
      continue;
    }

    if (r != 0)
      kwarn("%s exited with %d\n", path, r);
    else
      klog("%s exited with %d\n", path, r);
  }

  free(dirs);
}

/**
 * Execute the script given to us by BOOTP/DHCP
 */
void
st_init()
{
  int r;
  if (!*startup_script) {
    kwarn("No BP_PARM script could be obtained, skipping\n");
    return;
  }

  switch (script_get_type(startup_script)) {
  case SCRIPT_LUA:
    r = lua_exec_script(startup_script);
    break;
  default:
    kwarn("Unknown script type for '%s' -- Treating as Cexpsh...\n", startup_script);
  case SCRIPT_CEXPSH:
    r = cexpsh_exec_script(startup_script);
    break;
  }

  if (r != 0)
    kerror("Script exited with %d\n", r);
  else
    klog("Script exited with %d\n", r);
}

/**
 * Main RTEMS entry point
 */
void*
POSIX_Init(void *argument)
{
  /* Init nvram */
  nvram_init();

  /* Init serial console */
  serial_init();

  /* Unpack the rootfs */
  imfs_init();

  /* Prompt for debug shell */
  earlyshell_prompt();

  /* Run /etc/rc.lua/cmd */
  rc_init();
  
#ifndef RTI_CONFIG_SKIP_NETWORK
  /* Setup network, dispatch dhcp */
  network_init();
#endif

  /* setup PATH */
  path_init();

  /* Setup remote mounts */
  mounts_init();

  /* Kick off scripts in /etc/init.d */
  initd_init();

  /* Execute the startup script given by dhcp */
  st_init();

#ifdef RTI_CONFIG_TESTS_ON_BOOT
  run_tests();
#endif
  
  /* Start interactive shell */
  shell_init(false);
  return 0;
}

RTEMS_SYSINIT_ITEM(
  early_init,
  RTEMS_SYSINIT_BSP_EARLY,
  RTEMS_SYSINIT_ORDER_MIDDLE
);

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
