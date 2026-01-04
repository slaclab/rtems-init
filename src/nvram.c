/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Utilities for interacting with NVRAM
 * ----------------------------------------------------------------------------
 * Notes:
 *    - Does not support mvme3100, which uses an i2c based nvram
 *    - Does not support uC5282
 *    - Does not support writing to nvram
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
#include <bsp.h>
#include <rtems/bspcmdline.h>

#include <stddef.h>
#include <stdint.h>
#include <ctype.h>

#include "rtems-init.h"
#include "common/util.h"

#if defined(BSP_beatnik) || defined(BSP_mvme3100)
#define MOTLOAD_OFFSET 0x7000
#define MOTLOAD_HEADER_SIZE 0xF8
#define MOTLOAD_GEV_SIZE 3592
#endif

/** Nvram signature/version; pulled from rtems-netboot */
#define NVRAM_SIGN 0xcafe
#define NVRAM_SIGN_SIZE (2*sizeof(uint16_t))

/** List of valid NVRAM params, for systems where we can't arbitrarily iterate them */
const char* bootp_params[] =
{
  "BP_MYIP",
  "BP_GTWY",
  "BP_MYMK",
  "BP_MYNM",
  "BP_MYDN",
  "BP_LOGH",
  "BP_DNS1",
  "BP_DNS2",
  "BP_DNS3",
  "BP_NTP1",
  "BP_NTP2",
  "BP_NTP3",
  "BP_ENBL",
  "BP_DELY",
  "BP_PARM",
  "BP_FILE",
  NULL,
};

/**
 * nvram boot parameters are set by Till's netboot system. Unlike GEVs,
 * this is just a string stored in nvram. The format is:
 *  MY_PARAM='some data'
 * As far as I know, all data values are bounded by quotes, and the
 * variable name is always separated using an =
 */
static int
parse_boot_string(
  const char* pboot,
  const char* pend,
  const char* param, 
  char* result,
  size_t resultsz
)
{
  while (pboot < pend) {
    int quoted = 0;

    /** Skip leading space */
    while (*pboot && isspace((int)*pboot) && pboot < pend)
      pboot++;
    
    /** End of nvram */
    if (!*pboot)
      break;

    /** Scan until = token */
    const char* pstart = pboot;
    while (*pboot && *pboot != '=' && pboot<pend)
      pboot++;
    
    if (!*pboot || pboot == pend)
      break;
    
    /** Compare parameter name (pboot is '=' currently) */
    if (!strncmp(param, pstart, max(pboot-pstart, strlen(param)))) {
      pboot++;
      if (*pboot == '\'') quoted = 1, pboot++;
      if (pboot >= pend) break;
      /** Eat until closing quote */
      size_t n = resultsz;
      char* p = result;
      while ((quoted ? *pboot != '\'' : *pboot != ' ') 
              && *pboot && pboot < pend && n > 0) {
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
      pboot++;
      if (*pboot == '\'') quoted = 1, pboot++;
      if (pboot >= pend) break;
      while ((quoted ? *pboot != '\'' : *pboot != ' ') && *pboot &&  pboot < pend)
        pboot++;
      pboot++;
    }
  }
  
  return -1; //nvram_parse_param(pboot, pend, param, result, resultsz, 0);
}

static int
parse_boot_foreach(
  const char* pboot,
  const char* pend,
  void(*parsed)(const char*, size_t, const char*, size_t)
)
{
  size_t pl = 0, vl = 0;
  while (pboot < pend) {
    int quoted = 0;

    /** Skip leading space */
    while (*pboot && isspace((int)*pboot) && pboot < pend)
      pboot++;
    
    /** End of nvram */
    if (!*pboot)
      break;

    /** Scan until = token */
    const char* pstart = pboot;
    while (*pboot && *pboot != '=' && pboot<pend)
      pboot++;
    
    if (!*pboot || pboot >= pend)
      break;

    /** Compute parameter length; pboot is currently '=' */
    pl = pboot-pstart;

    /** Skip = and ' (if it's there) */
    pboot++;
    if (*pboot == '\'') quoted = 1, pboot++;
    if (pboot >= pend) break;
    const char* vs = pboot;

    /** Scan until closing quote or space */
    while ((quoted ? *pboot != '\'' : *pboot != ' ') && *pboot && pboot < pend)
      pboot++;

    /** Compute value length, excluding quote */
    vl = pboot - vs;

    parsed(pstart, pl, vs, vl);

    pboot++;
  }
  return 0;
}

#ifdef HAVE_NVRAM

int
boot_param(const char* param, char* result, size_t resultsz)
{
#ifdef BSP_uC5282
  const char* s = bsp_getbenv(param);
  strncpySafe(result, s, resultsz);
  return 0;
#elif defined(BSP_NVRAM_BOOTPARMS_START)
  const char* pboot = (const char*)BSP_NVRAM_BOOTPARMS_START;
  const char* pend = (const char*)BSP_NVRAM_BOOTPARMS_END;
  pboot += NVRAM_SIGN_SIZE;
  return parse_boot_string(pboot, pend, param, result, resultsz);
#else
  return 1;
#endif
}

int
boot_param_foreach(void(*parsed)(const char*, size_t, const char*, size_t))
{
#ifdef BSP_uC5282
  const char* s;
  for (int i = 0; i < sizeof(bootp_params) / sizeof(bootp_params[0]); ++i)
    if (bootp_params[i] && (s = bsp_getbenv(bootp_params[i])))
      parsed(bootp_params[i], strlen(bootp_params[i]), s, strlen(s));
  return 0;
#elif defined(BSP_NVRAM_BOOTPARMS_START)
  const char* pboot = (const char*)BSP_NVRAM_BOOTPARMS_START;
  const char* pend = (const char*)BSP_NVRAM_BOOTPARMS_END;
  pboot += NVRAM_SIGN_SIZE;
  return parse_boot_foreach(pboot, pend, parsed);
#else
  return 1;
#endif
}

int
boot_param_show_all()
{
#ifdef BSP_uC5282
  const char* s;
  for (int i = 0; i < sizeof(bootp_params) / sizeof(bootp_params[0]); ++i)
    if (bootp_params[i] && (s = bsp_getbenv(bootp_params[i])))
      printf("%s=%s\n", bootp_params[i], s);
  return 0;
#elif defined(BSP_NVRAM_BOOTPARMS_START)
  const char* pboot = (const char*)BSP_NVRAM_BOOTPARMS_START;
  const char* pend = (const char*)BSP_NVRAM_BOOTPARMS_END;
  pboot += NVRAM_SIGN_SIZE;
  /** Boot params are simply a string, can just directly print */
  puts(pboot);
  return 0;
#else
  return 1;
#endif
}

#endif // HAVE_NVRAM

#ifdef HAVE_MOTLOAD
/**
 * Lookup a single nvram gev parameter based on name. Not very efficient, I know.
 */
int
gev_param(const char* param, char* result, size_t resultsz)
{
#ifdef BSP_NVRAM_BASE_ADDR
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
#else
  return -1;
#endif
}

/**
 * Bulk display of nvram parameters
 */
int
gev_show()
{
#ifdef BSP_NVRAM_BASE_ADDR
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
#else
  return -1;
#endif
}

#endif // HAVE_MOTLOAD

int
nvram_get_boot_param(const char* param, char* res, size_t n)
{
#ifdef HAVE_NVRAM
  return boot_param(param, res, n);
#else
  return -1;
#endif
}

/*************** Shell commands ***************/

int
shell_nvram_get(int argc, char** argv)
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s param-name\n", argv[0]);
    return -1;
  }

#ifdef HAVE_NVRAM
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
#ifdef HAVE_NVRAM
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

static void
put_param_env(const char* param, size_t pl, const char* val, size_t vl)
{
  char sparam[256];
  strncpySafe(sparam, param, min(sizeof(sparam), pl+1));

  char sval[1024];
  strncpySafe(sval, val, min(sizeof(sval), vl+1));

  setenv(sparam, sval, 1);
}

static int
cmdline_foreach(void(*parsed)(const char*, size_t, const char*, size_t))
{
  const char* cmdline = rtems_bsp_cmdline_get();
  if (!cmdline) return -1;
  
  /** Cache the cmdline length, it shouldn't change */
  static int len = -1;
  if (len < 0)
    len = strlen(cmdline);

  return parse_boot_foreach(cmdline, cmdline+len, parsed);
}

int
nvram_init()
{
#ifdef HAVE_NVRAM
  boot_param_foreach(put_param_env);
#endif
  cmdline_foreach(put_param_env);
  return 0;
}

int
cmdline_get_param(const char* param, char* res, size_t n)
{
  const char* cmdline = rtems_bsp_cmdline_get();
  if (!cmdline) return -1;
  
  /** Cache the cmdline length, it shouldn't change */
  static int len = -1;
  if (len < 0)
    len = strlen(cmdline);
  
  return parse_boot_string(cmdline, cmdline+len, param, res, n);
}
