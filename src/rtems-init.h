/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: rtems-init main header
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
#pragma once

#include <rtems/shell.h>

#if defined(BSP_beatnik) || defined(BSP_mvme3100)
# define HAVE_MOTLOAD
# define RTEMS_NETWORK_CONFIG_MBUF_SPACE 2048
# define RTEMS_NETWORK_CONFIG_CLUSTER_SPACE 5120
# define HAVE_NVRAM
# define HAVE_MOTLOAD
#elif defined(__i386__)
# define RTEMS_NETWORK_CONFIG_MBUF_SPACE 2048
# define RTEMS_NETWORK_CONFIG_CLUSTER_SPACE 5120
#elif defined(BSP_uC5282)
# define RTEMS_NETWORK_CONFIG_MBUF_SPACE 200
# define RTEMS_NETWORK_CONFIG_CLUSTER_SPACE 350
# define HAVE_NVRAM
#else
# error Set RTEMS_NETWORK_CONFIG_MBUF_SPACE and RTEMS_NETWORK_CONFIG_CLUSTER_SPACE for your BSP!
#endif

struct dhcp_runtime_cfg
{
  char cmdline[1024];
  char filename[1024];
  char bootfile[1024];
  char interface[1024];
  char ntp1[64];
  char ntp2[64];
  char ntp3[64];
  char dns1[64];
  char dns2[64];
  char dns3[64];
  char domain[128];
  char hostname[128];
  char tftp_server[128];
  char timezone[64];
  
  struct _event_s* event;
};

enum init_mode {
  INIT_MODE_CMDLINE,  /**< Use command line supplied by netboot */
  INIT_MODE_NVRAM,    /**< Use raw params stored in nvram */
  INIT_MODE_DHCP,     /**< Use DHCP */
};

extern struct dhcp_runtime_cfg dhcp_runtime_cfg;

extern int verbose;
extern enum init_mode init_mode;

/********************************************************
 * shell.c
 ********************************************************/

typedef struct shell_cmd
{
  const char*           cmd;
  const char*           topic;
  const char*           usage;
  rtems_shell_command_t command;
} shell_cmd_t;

extern struct shell_cmd shell_cmds[];

/********************************************************
 * rootfs.c (generated)
 ********************************************************/

extern void unpack_rootfs();

extern unsigned char tar_rootfs[];
extern size_t tar_rootfs_SIZE;

/********************************************************
 * fdt (generated)
 ********************************************************/

extern unsigned char system_dtb[];
extern size_t system_dtb_SIZE;

/********************************************************
 * nvram.c
 ********************************************************/

extern int nvram_init();
extern int nvram_get_boot_param(const char* param, char* res, size_t n);
extern int cmdline_get_param(const char* param, char* res, size_t n);

extern int shell_nvram_get(int argc, char** argv);
extern int shell_nvram_show(int argc, char** argv);
extern int shell_gev_get(int argc, char** argv);
extern int shell_gev_show(int argc, char** argv);

/********************************************************
 * net.c
 ********************************************************/

extern int generate_resolv_conf();
extern void tz_init();
extern int ntp_init();

/********************************************************
 * net_bsd.c OR net_legacy.c
 ********************************************************/

extern void network_init();

/********************************************************
 * rtems-lua.c
 ********************************************************/

extern int lua_exec_script(const char* file);

/********************************************************
 * util.c
 ********************************************************/

/* Returns 1 if file exists */
int file_exists(const char* file);

enum klog_color {
  KLOG_NONE,
  KLOG_GREEN,
  KLOG_YELLOW,
  KLOG_RED,
  KLOG_BLUE,
};

/* klog helpers */
int kvlog(enum klog_color c, const char* fmt, va_list va);
int kclog(enum klog_color c, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
int klog(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
int kvwarn(const char* fmt, va_list va);
int kwarn(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
int kverror(const char* fmt, va_list va);
int kerror(const char* fmt, ...) __attribute__((format(printf, 1, 2)));