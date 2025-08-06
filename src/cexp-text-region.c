/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Cexpsh text region configuration
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

/* Use 16M text region for PPC */
#ifdef __PPC__
#define RTEMS_CEXP_TEXT_REGION_SIZE 0x01000000
#endif

#if defined(RTEMS_CEXP_TEXT_REGION_SIZE) && RTEMS_CEXP_TEXT_REGION_SIZE > 0
char cexpTextRegion[RTEMS_CEXP_TEXT_REGION_SIZE] = {0};
unsigned long cexpTextRegionSize = RTEMS_CEXP_TEXT_REGION_SIZE;
#else
unsigned long cexpTextRegionSize = 0;
#endif
