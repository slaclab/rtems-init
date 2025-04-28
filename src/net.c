/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Common networking code
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int
generate_resolv_conf()
{
  FILE* fp = fopen("/etc/resolv.conf", "wb");
  if (!fp) {
    printf("*** Failed to create /etc/resolv.conf\n");
    return -1;
  }

  const char* d = NULL;
  if ((d = getenv("BP_DNS1")))
    fprintf(fp, "nameserver %s\n", d);
  if ((d = getenv("BP_DNS2")))
    fprintf(fp, "nameserver %s\n", d);
  if ((d = getenv("BP_DNS3")))
    fprintf(fp, "nameserver %s\n\n", d);
  if ((d = getenv("BP_MYDN")))
    fprintf(fp, "search %s\n", d);

  fclose(fp);
  return 0;
}
