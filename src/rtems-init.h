
#pragma once

#include <rtems/shell.h>

#if defined(BSP_beatnik) || defined(BSP_mvme3100)
#define HAVE_MOTLOAD
#define RTEMS_NETWORK_CONFIG_MBUF_SPACE 2048
#define RTEMS_NETWORK_CONFIG_CLUSTER_SPACE 5120
#endif

struct dhcp_runtime_cfg
{
  char cmdline[1024];
  char filename[1024];
  char bootfile[1024];
  char interface[1024];
  char ntp1[128];
  char ntp2[128];
  char ntp3[128];
  char hostname[128];
  char tftp_server[128];
  
  struct _event_s* event;
};

extern struct dhcp_runtime_cfg dhcp_runtime_cfg;

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

/********************************************************
 * nvram.c
 ********************************************************/

extern int nvram_get_boot_param(const char* param, char* res, size_t n);

extern int shell_nvram_get(int argc, char** argv);
extern int shell_nvram_show(int argc, char** argv);
extern int shell_gev_get(int argc, char** argv);
extern int shell_gev_show(int argc, char** argv);

/********************************************************
 * ntp.c
 ********************************************************/

extern int ntp_init();
extern void ntp_shutdown();