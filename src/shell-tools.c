/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Misc shell utilities for Cexpsh
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

#include <rtems/shell.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/syslimits.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>

#ifdef HAVE_CEXP
#include <cexpHelp.h>
#define CEXP_COMMA ,
#else
#define CEXP_HELP_TAB_BEGIN(...)
#define HELP(...)
#define CEXP_HELP_TAB_END
#define CEXP_COMMA
#endif

int
sh()
{
  return rtems_shell_main_loop(rtems_shell_get_current_env());
}

CEXP_HELP_TAB_BEGIN(sh)
	HELP(
    "Run an instance of the RTEMS shell\n",
	  int, sh,  (void)
	)CEXP_COMMA
CEXP_HELP_TAB_END

int
cat(const char* path)
{
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return -1;
  }
  
  char buf[4096];
  ssize_t r;
  while ((r = read(fd, buf, sizeof(buf))) > 0) {
    write(STDOUT_FILENO, buf, r);
  }

  close(fd);
  return 0;
}

CEXP_HELP_TAB_BEGIN(cat)
	HELP(
    "Print a file to stdout\n",
	  int, cat,  (const char*)
	)CEXP_COMMA
CEXP_HELP_TAB_END

int
cp(const char* src, const char* dst)
{
  int ifd = open(src, O_RDONLY);
  if (ifd < 0) {
    perror("open");
    return -1;
  }

  int ofd = open(src, O_RDWR | O_CREAT, 0644);
  if (ofd < 0) {
    perror("open");
    close(ifd);
    return -1;
  }

  int ret = 0;
  ssize_t r;
  char buf[4096];

  while ((r = read(ifd, buf, sizeof(buf))) > 0) {
    if (write(ofd, buf, r) < 0) {
      perror("write");
      ret = -1;
      break;
    }
  }

  if (r < 0) {
    perror("read");
    ret = -1;
  }

  close(ofd);
  close(ifd);
  return ret;
}

CEXP_HELP_TAB_BEGIN(cp)
	HELP(
    "Copy a to b\n",
	  int, cp,  (const char* a, const char* b)
	)CEXP_COMMA
CEXP_HELP_TAB_END

int
ls(const char* dir)
{
  char buf[PATH_MAX];
  if (!dir)
    dir = getcwd(buf, sizeof(buf));
  else
    strcpy(buf, dir);

  DIR* d = opendir(dir);
  if (!d) {
    perror("opendir");
    return -1;
  }

  char cent[PATH_MAX], tgt[PATH_MAX];
  for (struct dirent* e = readdir(d); e; e = readdir(d)) {
    snprintf(cent, sizeof(cent), "%s/%s", buf, e->d_name);
    struct stat st;
    if (lstat(cent, &st) < 0) {
      printf("stat: %s: %s\n", cent, strerror(errno));
    }

    int islink = S_ISLNK(st.st_mode);
    if (islink) {
      if (readlink(cent, tgt, sizeof(tgt)) < 0)
        islink = 0;
    }

    printf(
      "0x%016llX, %8llub, %05d.%05d, %s%s%s\n",
      (long long unsigned)st.st_ino,
      (long long unsigned)st.st_size,
      st.st_uid,
      st.st_gid,
      e->d_name,
      islink ? " -> " : "",
      islink ? tgt : ""
    );
  }

  closedir(d);
  return 0;
}

CEXP_HELP_TAB_BEGIN(ls)
	HELP(
    "List directory entries\n",
	  int, ls,  (const char* dir)
	)CEXP_COMMA
CEXP_HELP_TAB_END

void
dumpEnv()
{
  char** s = environ;
  for (; s && *s; s++)
    puts(*s);
}

CEXP_HELP_TAB_BEGIN(dumpEnv)
	HELP(
    "Dump all environment entries\n",
	  void, dumpEnv,  (void)
	)CEXP_COMMA
CEXP_HELP_TAB_END

void
pwd()
{
  char buf[PATH_MAX];
  getcwd(buf, sizeof(buf));
  puts(buf);
}

CEXP_HELP_TAB_BEGIN(pwd)
	HELP(
    "Print current working directory\n",
	  void, pwd,  (void)
	)CEXP_COMMA
CEXP_HELP_TAB_END

/* Helper to match behavior from RTEMS 4.X */
int
nfsMount(const char* ip, const char* src, const char* mntpt)
{
  char opts[256] = {0};
  char newip[128];
  char source[512];

  /* skip UID.GID prefix, if any */
  /* TODO: handle uid,gid */
  const char* at = strchr(ip, '@');
  if (at)
    ip = at+1;

  /* resolve host */
  struct addrinfo* ai = NULL;
  struct addrinfo hint = {0};
  hint.ai_family = AF_INET;
  hint.ai_flags = AI_PASSIVE;
  if (getaddrinfo(ip, NULL, &hint, &ai) != 0) {
    printf("do_mount: addr lookup failed: %s\n", strerror(errno));
    return -1;
  }

  if (ai->ai_addr->sa_len != sizeof(struct sockaddr_in) || 
      ai->ai_addr->sa_family != AF_INET) {
    printf("do_mount: addr lookup failed, didn't get ipv4 addr\n");
    freeaddrinfo(ai);
    return -1;
  }

  struct sockaddr_in* si =
    (struct sockaddr_in*)ai->ai_addr;
  si->sin_addr.s_addr = ntohl(si->sin_addr.s_addr);
  snprintf(newip, sizeof(newip), "%u.%u.%u.%u",
    (si->sin_addr.s_addr & 0xFF000000) >> 24,
    (si->sin_addr.s_addr & 0x00FF0000) >> 16,
    (si->sin_addr.s_addr & 0x0000FF00) >> 8,
    (si->sin_addr.s_addr & 0x000000FF));

  freeaddrinfo(ai);

  /* Ensure mount point exists */
  rtems_mkdir(mntpt, 0777);

  snprintf(source, sizeof(source), "%s:%s", newip, src);

  strcpy(opts, "vers=3");

  if (mount(source, mntpt, RTEMS_FILESYSTEM_TYPE_NFS, 0, opts) < 0) {
    printf("mount: %s\n", strerror(errno));
    return -1;
  }

  return 0;
}

/* This symbol will be ref'ed by a linker script. Make sure your shell commands
 * are added here or they'll be culled! */
void* _shell_symbol_tbl[] =
{
  &nfsMount,
  &pwd,
  &dumpEnv,
  &ls,
  &cp,
  &cat,
  &sh
};
