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

#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

struct dhcp_runtime_cfg dhcp_runtime_cfg;

int verbose = 1;

enum init_mode init_mode = INIT_MODE_CMDLINE;

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
