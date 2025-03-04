
#include "util.h"

#include <sys/errno.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

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

static int
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