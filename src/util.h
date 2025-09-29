/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Common utilities
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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

/** ANSI escape codes */
#define ANSI_RED "\e[31m"
#define ANSI_GREEN "\e[32m"
#define ANSI_YELLOW "\e[33m"
#define ANSI_RESET "\e[0m"
#define ANSI_BLUE "\e[34m"

/** Returns true (1) if the string starts with the prefix */
static inline int
strHasPrefix(const char* str, const char* pfx) {
  return !strncmp(str, pfx, strlen(pfx));
}

/** Safe strncpy */
static inline char*
strncpySafe(char* str, const char* src, size_t len) {
  strncpy(str, src, len-1);
  str[len-1] = 0;
  return str;
}

static inline int
min(int a, int b)
{
  return a < b ? a : b;
}

static inline int
max(int a, int b)
{
  return a > b ? a : b;
}

/**
 * Append some text to a file, creates the file if it doesn't exist.
 */
extern int append_file(const char* file, const char* text);

extern ssize_t read_file(const char* file, char* buf, size_t bsize);

enum fstype {
  FS_TYPE_NFS2,
  FS_TYPE_NFS3,
  FS_TYPE_NFS4,
  FS_TYPE_9P,
};

#define MNT_STR_BUF_SZ 512

/**
 * \brief Given a mount specification string, parse it into its components
 * \param mntblock The mount string
 * \param fstype [Out] file system type
 * \param uid [Out] UID
 * \param gid [Out] GID
 * \param ip [Out] Buffer holding IP string
 * \param src [Out] Buffer holding source path
 * \param mntpt [Out] Buffer holding mount point string
 * \param file [Out] Buffer holding file path
 */
extern int
parse_mount_spec(
  const char* mntblock,
  enum fstype* fstype,
  uint32_t* uid,
  uint32_t* gid,
  char* ip,
  char* src,
  char* mntpt,
  char* file
);

/**
 * \brief Convert mount paths to normal paths
 * Ex: /sdf/sw:epics/base -> /sdf/sw/epics/base
 */
extern void convert_mount_path(const char* path, char* out, size_t outsz);

/**
 * \brief Strip the filename component off of a path. Basically anything after the
 * last path separator.
 */
extern void strip_filename(char* path);

/**
 * \brief Returns the extension of the file, or ""
 */
extern const char* path_get_extension(const char* path);

/**
 * \brief Returns 1 if a mountpoint is mounted
 */
extern int ismounted(const char* mntpt);

/**
 * \brief Returns a textual representation of the BSP's name
 */
extern const char* bsp_get_name();

enum script_type {
  SCRIPT_UNKNOWN = -1,
  SCRIPT_CEXPSH,
  SCRIPT_LUA
};

enum script_type script_get_type(const char* path);

typedef struct _event_s event_t;
extern event_t* event_create();
extern int event_wait(event_t* ev, uint64_t timeout_ms);
extern void event_signal(event_t* ev);
extern void event_destroy(event_t* ev);

extern void bsp_cmdline_get_param(const char* param, char* val, size_t vlen);
extern int bsp_cmdline_has_param(const char* param);