#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

/** Returns true (1) if the string starts with the prefix */
static inline int
strHasPrefix(const char* str, const char* pfx) {
  return !strncmp(str, pfx, strlen(pfx));
}

/** Safe strncpy */
static inline char*
strncpySafe(char* str, const char* src, size_t len) {
  strncpy(str, src, len);
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

extern int parse_mount_spec(const char* mntblock, enum fstype* fstype, uint32_t* uid,
  uint32_t* gid, char* ip, char* src, char* mntpt, char* file);

extern int ismounted(const char* mntpt);

typedef struct _event_s event_t;
extern event_t* event_create();
extern int event_wait(event_t* ev, uint64_t timeout_ms);
extern void event_signal(event_t* ev);
extern void event_destroy(event_t* ev);

