#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

/**
 * Append some text to a file, creates the file if it doesn't exist.
 */
static int append_file(const char* file, const char* text);

/** Generic event API */

typedef struct _event_s event_t;
extern event_t* event_create();
extern int event_wait(event_t* ev, uint64_t timeout_ms);
extern void event_signal(event_t* ev);
extern void event_destroy(event_t* ev);

