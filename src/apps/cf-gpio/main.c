/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Configuration for RTEMS
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

#include <mcf5282/mcf5282.h>

#include "common/rtems-test-config.h"
#include "util.h"

void *
POSIX_Init(void *argument)
{
  ios_shell_input(fileno(stdin), NULL);
  
  printf("cf-gpio -- built %s at %s\n", __DATE__, __TIME__);

  usleep(10000);

  //MCF5282_GPIO_PBCDPAR &= ~MCF5282_GPIO_PBCDPAR_PCDPA;
  //MCF5282_GPIO_PBCDPAR &= ~MCF5282_GPIO_PBCDPAR_PBPA;
  MCF5282_GPIO_DDRB = 0xFF;
  MCF5282_GPIO_DDRC = 0xFF;

  while (1) {
    MCF5282_GPIO_PORTB |= (vuint8)0xFF;
    MCF5282_GPIO_PORTC |= (vuint8)0xFF;
    usleep(1000);
    MCF5282_GPIO_PORTB &= 0;
    MCF5282_GPIO_PORTC &= 0;
    usleep(1000);
    printk("A\n");
  }
  return NULL;
}
