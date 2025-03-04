
#include <rtems.h>
#include <bsp.h>

#include <stddef.h>
#include <stdint.h>
#include <ctype.h>

#include "rtems-init.h"

#if defined(BSP_beatnik) || defined(BSP_mvme3100)
#define MOTLOAD_OFFSET 0x7000
#define MOTLOAD_HEADER_SIZE 0xF8
#define MOTLOAD_GEV_SIZE 3592
/** Nvram signature/version; pulled from rtems-netboot */
#define NVRAM_SIGN 0xcafe
#define NVRAM_SIGN_SIZE (2*sizeof(uint16_t))
#endif

#ifdef HAVE_MOTLOAD

int
boot_param(const char* param, char* result, size_t resultsz)
{
  const char* pboot = (const char*)BSP_NVRAM_BOOTPARMS_START;
  const char* pend = (const char*)BSP_NVRAM_BOOTPARMS_END;
  pboot += NVRAM_SIGN_SIZE;
  
  while (pboot < pend) {
    /** Skip leading space */
    while (*pboot && isspace(*pboot) && pboot < pend)
      pboot++;
    
    /** Scan until = token */
    const char* pstart = pboot;
    while (*pboot && *pboot != '=' && pboot<pend)
      pboot++;
    
    if (!*pboot || pboot == pend)
      break;
    
    /** Compare parameter name (pboot is '=' currently) */
    if (!strncmp(param, pstart, pboot-pstart-1)) {
      pboot += 2;
      if (pboot >= pend) break;
      /** Eat until closing quote */
      size_t n = resultsz;
      char* p = result;
      while (*pboot != '\'' && *pboot && pboot < pend && n > 0) {
        *p = *pboot;
        p++, pboot++, n--;
      }
      
      /** Ensure terminated correctly */
      if (n > 0)
        *p = 0;
      else
        result[resultsz-1] = 0;
      return 0;
    }
    /** No match; skip the parameter string */
    else {
      pboot += 2;
      if (pboot >= pend) break;
      while (*pboot && *pboot != '\'' && pboot < pend)
        pboot++;
      pboot++;
    }
  }
  
  return -1; //nvram_parse_param(pboot, pend, param, result, resultsz, 0);
}

int
boot_param_show_all()
{
  const char* pboot = (const char*)BSP_NVRAM_BOOTPARMS_START;
  const char* pend = (const char*)BSP_NVRAM_BOOTPARMS_END;
  pboot += NVRAM_SIGN_SIZE;
  /** Boot params are simply a string, can just directly print */
  puts(pboot);
  return 0;
}

/**
 * Lookup a single nvram gev parameter based on name. Not very efficient, I know.
 */
int
gev_param(const char* param, char* result, size_t resultsz)
{
  const char* pgev = 
    (char*)(BSP_NVRAM_BASE_ADDR + MOTLOAD_OFFSET + MOTLOAD_HEADER_SIZE);
  const char* const pgevend = pgev + MOTLOAD_GEV_SIZE;

  while (pgev < pgevend) {
    const char* pstart = pgev;
    /** Scan forward for = */
    while (*pgev && *pgev != '=' && (pgev<pgevend))
      pgev++;
    
    /** Failed to parse; probably end of nvram */
    if (*pgev != '=')
      break;
    
    if (!strncmp(pstart, param, pgev-pstart-1)) {
      /** Matched; copy gev+1 (gev is '=') until \0 into result */
      pgev++;
      char* p = result;
      size_t n = resultsz;
      while (*pgev && (pgev<pgevend) && n > 0) {
        *p = *pgev;
        n--, p++, pgev++;
      }
      
      /** If we are boot parameters, strip trailing ' */
      if (p>result && *(p-1) == '\'')
        *(p-1) = 0;

      /** Ensure terminated */
      if (p-result < resultsz)
        *p = 0;
      else
        result[resultsz-1] = 0;
      return 0;
    }
    /** No match, skip till next component (until \0) */
    else {
      while (*pgev && (pgev<pgevend))
        pgev++;
      /** Skip last \0 */
      if (pgev < pgevend)
        pgev++;
    }
  }
  return -1;
}

/**
 * Bulk display of nvram parameters
 */
int
gev_show()
{
  const char* pgev = 
    (char*)(BSP_NVRAM_BASE_ADDR + MOTLOAD_OFFSET + MOTLOAD_HEADER_SIZE);
  const char* const pgevend = pgev + MOTLOAD_GEV_SIZE;
  int n = 0;

  while (pgev < pgevend) {
    /** Skip \0 delimiters, but bail if we skip more than 2 */
    while (!*pgev && pgev < pgevend)
      pgev++, n++;
    if (n > 2)
      break;
    n = 0;
    printf("%s\n", pgev);
    /** Skip until next \0 */
    while (*pgev && pgev < pgevend)
      pgev++;
  }
  return 0;
}


#endif

int
nvram_get_boot_param(const char* param, char* res, size_t n)
{
  return boot_param(param, res, n);
}

/*************** Shell commands ***************/

int
shell_nvram_get(int argc, char** argv)
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s param-name\n", argv[0]);
    return -1;
  }

#ifdef HAVE_MOTLOAD
  char buf[512];
  if (boot_param(argv[1], buf, sizeof(buf)) < 0) {
    fprintf(stderr, "unable to find nvram parameter %s\n", argv[1]);
    return -1;
  }
  printf("%s\n", buf);
#else
  fprintf(stderr, "nvram unimplemented for this BSP\n");
  return -1;
#endif
  return 0;
}

int
shell_nvram_show(int argc, char** argv)
{
#ifdef HAVE_MOTLOAD
  boot_param_show_all();
#else
  fprintf(stderr, "nvram unimplemented for this BSP\n");
  return -1;
#endif
  return 0;
}

int
shell_gev_get(int argc, char** argv)
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s param-name\n", argv[0]);
    return -1;
  }

#ifdef HAVE_MOTLOAD
  char buf[512];
  if (gev_param(argv[1], buf, sizeof(buf)) < 0) {
    fprintf(stderr, "unable to find gev nvram parameter %s\n", argv[1]);
    return -1;
  }
  printf("%s\n", buf);
#else
  fprintf(stderr, "nvram unimplemented for this BSP\n");
  return -1;
#endif
  return 0;
}

int
shell_gev_show(int argc, char** argv)
{
#ifdef HAVE_MOTLOAD
  gev_show();
#else
  fprintf(stderr, "nvram unimplemented for this BSP\n");
  return -1;
#endif
  return 0;
}