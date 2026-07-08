/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Common utilities. These can be used standalone.
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
#include <rtems/bspcmdline.h>
#include <bsp.h>

#include <sys/errno.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <termios.h>

#include "util.h"

#define KLOG_STREAM stderr

struct _event_s
{
  pthread_cond_t cond;
  pthread_condattr_t condattr;
  pthread_mutex_t mutex;
  pthread_mutexattr_t mutexattr;
};

event_t*
event_create()
{
  event_t *ev = malloc(sizeof(event_t));
  memset(ev, 0, sizeof *ev);
  pthread_condattr_init(&ev->condattr);
  pthread_cond_init(&ev->cond, &ev->condattr);
  pthread_mutexattr_init(&ev->mutexattr);
  pthread_mutex_init(&ev->mutex, &ev->mutexattr);
  return ev;
}

int
event_wait(event_t *ev, uint64_t timeout_ms)
{
  struct timespec tv = {};
  clock_gettime(CLOCK_REALTIME, &tv);
  uint64_t ns = tv.tv_nsec + timeout_ms * 1e6;
  tv.tv_nsec = ns % 1000000000ULL;
  tv.tv_sec += (ns / 1000000000ULL);

  pthread_mutex_lock(&ev->mutex);
  int r = 0;
  do {
  } while ((r = pthread_cond_timedwait(&ev->cond, &ev->mutex, &tv)) != 0 &&
      (r == EAGAIN || r == EINTR));

  pthread_mutex_unlock(&ev->mutex); // Dont need to hold this mutex

  return r;
}

void
event_signal(event_t *ev)
{
  pthread_mutex_lock(&ev->mutex);
  pthread_cond_broadcast(&ev->cond);
  pthread_mutex_unlock(&ev->mutex);
}

void 
event_destroy(event_t *ev)
{
  if (!ev)
    return;
  pthread_cond_destroy(&ev->cond);
  pthread_condattr_destroy(&ev->condattr);
  pthread_mutex_destroy(&ev->mutex);
  pthread_mutexattr_destroy(&ev->mutexattr);
  free(ev);
}

int
append_file(const char* file, const char* text)
{
  int fd = open(file, O_WRONLY | O_CREAT, 0644);
  if (fd < 0)
    return -1;
  
  if (lseek(fd, 0, SEEK_END) < 0) {
    close(fd);
    return -1;
  }

  int r = write(fd, text, strlen(text));
  close(fd);
  return r;
}

ssize_t
read_file(const char* file, char* buf, size_t bsize)
{
  int fd = open(file, O_RDONLY);
  if (fd < 0)
    return -1;

  ssize_t r = read(fd, buf, bsize);
  close(fd);
  return r;
}

int 
parse_mount_spec(const char* mntblock, enum fstype* fstype, uint32_t* uid,
  uint32_t* gid, char* ip, char* src, char* mntpt, char* file)
{
  int skip = 0;
  if (strHasPrefix(mntblock, "nfs2::"))
    skip = 6, *fstype = FS_TYPE_NFS2;
  else if (strHasPrefix(mntblock, "nfs3::"))
    skip = 6,*fstype = FS_TYPE_NFS3;
  else if (strHasPrefix(mntblock, "nfs::"))
    skip = 5,*fstype = FS_TYPE_NFS3;
  else if (strHasPrefix(mntblock, "nfs4::"))
    skip = 6, *fstype = FS_TYPE_NFS4;
  else if (strHasPrefix(mntblock, "9p::"))
    skip = 4, *fstype = FS_TYPE_9P;
  else
    *fstype = FS_TYPE_NFS3;
  mntblock += skip;

  /** Try to find optional 1234.4567@ prefix */
  const char* p = strpbrk(mntblock, "@");
  if (p) {
    const char* g = strpbrk(mntblock, ".");
    /** Must be lone uid */
    if (!g || g > p) {
      char* ep = p-1;
      *uid = strtol(mntblock, &ep, 10);
    }
    /** full uid.gid */
    else {
      char* ep = g-1;
      *uid = strtol(mntblock, &ep, 10);
      ep = p-1;
      *gid = strtol(g+1, &ep, 10);
    }
    mntblock = p+1;
  }

  /** Find end of server decl */
  const char* srvend = strchr(mntblock, ':');

  if (!srvend)
    return -1;

  strncpySafe(ip, mntblock, min(MNT_STR_BUF_SZ, srvend-mntblock+1));
  mntblock = srvend+1;

  srvend = strchr(mntblock, ':');
  if (!srvend)
    return -1;

  strncpySafe(src, mntblock, min(MNT_STR_BUF_SZ, srvend-mntblock+1));
  mntblock = srvend+1;

  /** Next part is optional; it may be either file name or the mountpoint */
  const char* mntend = strchr(mntblock, ':');
  if (!mntend) {
    *file = *mntpt = 0;
    /** mountpoint the same as the source */
    strncpySafe(mntpt, src, MNT_STR_BUF_SZ);
    /** And the remaining bytes are the file */
    strncpySafe(file, mntblock, MNT_STR_BUF_SZ);
    return 0;
  }

  /** If we still have remaining text, this is the file name */
  if (*(mntend+1)) {
    strncpySafe(file, mntend+1, MNT_STR_BUF_SZ);
    /** And the previous block was mount point */
    strncpySafe(mntpt, mntblock, min(MNT_STR_BUF_SZ, mntend-mntblock+1));
  }
  /** Otherwise, the previous block was file */
  else {
    strncpySafe(file, mntblock, min(MNT_STR_BUF_SZ, mntend-mntblock+1));
  }

  return 0;
}

int
ismounted(const char* mntpt)
{
  struct stat child;
  if (stat(mntpt, &child) < 0)
    return 0;

  char par[256];
  strncpySafe(par, mntpt, sizeof(par));

  char* p = strrchr(par, '/');
  if (!p)
    return 0;
  *p = 0;

  struct stat parent;
  if (stat(par, &parent) < 0)
    return 0;

  return child.st_dev != parent.st_dev;
}

void
bsp_cmdline_get_param(const char* param, char* val, size_t vlen)
{
  rtems_bsp_cmdline_get_param(param, val, vlen);
}

int
bsp_cmdline_has_param(const char* param)
{
  return NULL != rtems_bsp_cmdline_get_param_raw(param);
}

int
file_exists(const char* file)
{
  struct stat st;
  if (stat(file, &st) < 0)
    return 0;
  return 1;
}

int
kvlog(enum klog_color c, const char* fmt, va_list va)
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);

  const char* nc = 0;
  switch (c) {
  case KLOG_GREEN:
    nc = ANSI_GREEN; break;
  case KLOG_YELLOW:
    nc = ANSI_YELLOW; break;
  case KLOG_RED:
    nc = ANSI_RED; break;
  case KLOG_BLUE:
    nc = ANSI_BLUE; break;
  default:
    break;
  }

  fprintf(KLOG_STREAM, ANSI_GREEN "[%5lld.%06ld] " ANSI_RESET,
    tp.tv_sec, tp.tv_nsec / 1000);
  if (nc)
    fprintf(KLOG_STREAM, "%s", nc);
  return vfprintf(KLOG_STREAM, fmt, va);
}

int
kclog(enum klog_color c, const char* fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int r = kvlog(c, fmt, va);
  va_end(va);
  return r;
}

int
klog(const char* fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int r = kvlog(KLOG_NONE, fmt, va);
  va_end(va);
  return r;
}

int
kvwarn(const char* fmt, va_list va)
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);

  fprintf(KLOG_STREAM, ANSI_GREEN "[%5lld.%06ld] " ANSI_YELLOW,
    tp.tv_sec, tp.tv_nsec / 1000);
  int r = vfprintf(KLOG_STREAM, fmt, va);
  fputs(ANSI_RESET, KLOG_STREAM);
  return r;
}

int
kwarn(const char* fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int r = kvwarn(fmt, va);
  va_end(va);
  return r;
}

int
kverror(const char* fmt, va_list va)
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);

  fprintf(KLOG_STREAM, ANSI_GREEN "[%5lld.%06ld] " ANSI_RED,
    tp.tv_sec, tp.tv_nsec / 1000);
  int r = vfprintf(KLOG_STREAM, fmt, va);
  fputs(ANSI_RESET, KLOG_STREAM);
  return r;
}

int
kerror(const char* fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int r = kverror(fmt, va);
  va_end(va);
  fputs(ANSI_RESET, KLOG_STREAM);
  return r;
}

void
convert_mount_path(const char* path, char* out, size_t outsz)
{
  strncpy(out, path, outsz-1);
  out[outsz-1] = 0;

  for (char* p = out; *p; ++p)
    if (*p == ':') *p = '/';
}

void
strip_filename(char* path)
{
  size_t l = strlen(path);
  char* e = path+l-1;
  while (e > path) {
    if (*e == '/') {
      while (*e == '/') {
        *e = 0; --e;
      }
      return;
    }
    --e;
  }
  *e = 0;
}

const char*
path_get_extension(const char* path)
{
  const char* c = strrchr(path, '.');
  if (!c) return "";
  return c+1;
}

enum script_type
script_get_type(const char* path)
{
  const char* ext = path_get_extension(path);
  if (!strcasecmp(ext, "lua"))
    return SCRIPT_LUA;
  else if (!strcasecmp(ext, "cmd") || !strcasecmp(ext, "cexp"))
    return SCRIPT_CEXPSH;
  return SCRIPT_UNKNOWN;
}

const char*
bsp_get_name()
{
#ifdef LIBBSP_BEATNIK_BSP_H
  return "mvme6100";
#elif defined(LIBBSP_POWERPC_MVME3100_BSP_H)
  return "mvme3100";
#elif defined(LIBBSP_I386_PC386_BSP_H)
  return "pc686";
#elif defined(LIBBSP_M68K_UC5282_BSP_H)
  return "uC5282";
#elif defined(LIBBSP_AARCH64_XILINX_ZYNQMP_BSP_H)
  return "ZynqMP";
#else
  #error "Please add an entry to bsp_get_name"
#endif
}

int
ios_immediate_input(int fd, struct termios* ptr)
{
  struct termios r = {0};
  int c = 0;
  if ((c = tcgetattr(fd, &r)) < 0)
    return c;
  
  struct termios new = r;
  new.c_lflag &= ~ICANON;
  /* configure for effectively nonblock IO */
  new.c_cc[VMIN] = 0;  /* min chars to 0 */
  new.c_cc[VTIME] = 0; /* timeout to 0 */

  if ((c = tcsetattr(fd, TCSANOW, &new)) < 0)
    return c;

  if (ptr)
    *ptr = r;
  return 0;
}

void
ios_restore(int fd, const struct termios* ptr)
{
  if (!ptr)
    return;
  
  int r = tcsetattr(fd, TCSANOW, ptr);
  if (r < 0)
    perror("tcsetattr");
}

int
ios_shell_input(int fd, struct termios* ptr)
{
  int r;

  struct termios tio;
  if ((r = tcgetattr(fd, &tio)) < 0) {
    kerror("tcgetattr: %s\n", strerror(errno));
    return r;
  }

  if (ptr)
    *ptr = tio;

  tio.c_iflag &= (IXOFF|IXON|IXANY|IGNBRK);
  tio.c_iflag |= BRKINT;
  tio.c_lflag |= ISIG;
  if (tcsetattr(fd, TCSANOW, &tio) < 0) {
    kerror("tcsetattr: %s\n", strerror(errno));
    return -1;
  }

  return 0;
}