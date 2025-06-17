/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: NFS performance testing code
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define NFS_FILE_PATH "/sdf/group/cds/sw/epics/users/lorelli/testfile.bin"
#define BLOCK_SIZE 4096
#define TEST_ITERS 1024

static float
time_now()
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return (float)tp.tv_sec + tp.tv_nsec / 1e9;
}

int
nfs_perf_test()
{
  float start, end;

  int ret = 0;
  int fd = open(NFS_FILE_PATH, O_RDWR | O_CREAT, 0777);
  if (fd < 0) {
    perror("open write");
    return -1;
  }
  
  uint8_t* data = malloc(BLOCK_SIZE);
  for (int i = 0; i < BLOCK_SIZE; ++i) {
    data[i] = i;
  }

  start = time_now();
  ssize_t written = 0;
  for (int i = 0; i < TEST_ITERS; ++i) {
    ssize_t r = write(fd, data, BLOCK_SIZE);
    if (r < 0) {
      perror("write");
      ret = 1;
      break;
    }
    written += r;
  }
  end = time_now();

  close(fd);

  printf("Wrote %.2f MiB in %.2f seconds (%.2f MiB/s)\n", written / (1024. * 1024.),
    end-start, (written / (1024.*1024.))/(end-start));
  
  if ((fd = open(NFS_FILE_PATH, O_RDONLY)) < 0) {
    perror("open read");
    free(data);
    return -1;
  }
  
  start = time_now();
  ssize_t readed = 0;
  for (int i = 0; i < TEST_ITERS; ++i) {
    ssize_t r = read(fd, data, BLOCK_SIZE);
    if (r < 0) {
      perror("read");
      ret = 1;
      break;
    }
    readed += r;
  }
  end = time_now();
  
  close(fd);
  free(data);
  
  printf("Read %.2f MiB in %.2f seconds (%.2f MiB/s)\n", readed / (1024. * 1024.),
    end-start, (readed / (1024.*1024.))/(end-start));
  
  return ret;
}