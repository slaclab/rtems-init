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

  char cent[PATH_MAX];
  for (struct dirent* e = readdir(d); e; e = readdir(d)) {
    snprintf(cent, sizeof(cent), "%s/%s", buf, e->d_name);
    struct stat st;
    if (stat(cent, &st) < 0) {
      perror("stat");
    }
    
    printf(
      "%8llu, %8llub, %05d.%05d, %s\n",
      (long long unsigned)st.st_ino,
      (long long unsigned)st.st_size,
      st.st_uid,
      st.st_gid,
      e->d_name
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
rtemsEntryPoint()
{
}

