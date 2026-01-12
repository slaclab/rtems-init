/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: IRQ test for MVME3100
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
#include <bsp/irq.h>
#include <bsp/irq-generic.h>
#include <assert.h>
#include <stdio.h>
#include <bsp/VME.h>

static int s_interrupts = 0;

#define TIMER_IRQ (BSP_MISC_IRQ_LOWEST_OFFSET + 5)

static volatile uint32_t* GTBCR_REG[4] = {
  (volatile uint32_t*)0xE1041110,
  (volatile uint32_t*)0xE1041150,
  (volatile uint32_t*)0xE1041190,
  (volatile uint32_t*)0xE10411D0
};

static volatile uint32_t* GTVPR_REG[4] = {
  (volatile uint32_t*)0xE1041120,
  (volatile uint32_t*)0xE1041160,
  (volatile uint32_t*)0xE10411A0,
  (volatile uint32_t*)0xE10411E0
};

static struct {
  uint32_t gtvpr;
  uint32_t gtbcr;
} s_configs[4];

static void
conf_timer(int which)
{
  assert(which < 4 && which >= 0);
  
  volatile uint32_t* gtvpr = GTVPR_REG[which];
  volatile uint32_t* gtbcr = GTBCR_REG[which];
  s_configs[which].gtbcr = *gtbcr;
  s_configs[which].gtvpr = *gtvpr;
  
  /* mask off interrupts for now */
  (*gtvpr) = 0x80000000;

  /* count some kinda large number of cycles */
  (*gtbcr) = 0xFFFF;
  (*gtbcr) &= ~0x1; /* clear count inhibit */

  /* configure prio and vector, clear mask */
  (*gtvpr) |= TIMER_IRQ & 0xFFFF; //<< 16;
  (*gtvpr) |= 0x4 << 16; //<< 12;

  /* unmask */
  (*gtvpr) &= ~0x80000000; //~0x1;

  fflush(stdout);
  printf("GTVPR%d: 0x%08X\n", which, *gtvpr);
  printf("GTBCR%d: 0x%08X\n", which, *gtbcr);
  printf("TCR: 0x%08X\n", *(volatile uint32_t*)0x41300);
}

static void
restore_timer(int which)
{
  assert(which < 4 && which >= 0);
  
  volatile uint32_t* gtvpr = GTVPR_REG[which];
  volatile uint32_t* gtbcr = GTBCR_REG[which];
  *gtvpr = s_configs[which].gtvpr;
  *gtbcr = s_configs[which].gtbcr;
}

static void
cnt_isr(void* p)
{
  printk("ISR\n");
  s_interrupts++;
  if (s_interrupts > 10) {
    rtems_id* id = p;
    if (rtems_event_send(*id, RTEMS_EVENT_13) != RTEMS_SUCCESSFUL) {
      printk("rtems_event_send failed\n");
    }
  }
}

/* New style IRQ test using the rtems_interrupt API */
int
irq_test()
{
  rtems_id me = rtems_task_self();

  int r = 0;
  //int r = rtems_interrupt_handler_install(
  //  TIMER_IRQ,
  //  "Timer2",
  //  RTEMS_INTERRUPT_UNIQUE,
  //  cnt_isr,
  //  &me
  //);
  
  rtems_irq_connect_data data = {
    .name = TIMER_IRQ,
    .hdl = cnt_isr,
    .handle = &me,
    .isOn = true,
  };
  BSP_install_rtems_irq_handler(&data);

  if (r != RTEMS_SUCCESSFUL) {
    printk("rtems_interrupt_handler_install: %d\n", r);
    return -1;
  }

  conf_timer(2);

  rtems_event_set evs;
  r = rtems_event_receive(
    RTEMS_EVENT_13,
    RTEMS_DEFAULT_OPTIONS,
    rtems_clock_get_ticks_per_second() * 10000,
    &evs
  );

  if (r != RTEMS_SUCCESSFUL) {
    printk("Test failed; events timed out!\n");
    r = -1;
  }
  else {
    printk("IRQ test passed\n");
  }

  rtems_interrupt_handler_remove(TIMER_IRQ, cnt_isr, &me);

  restore_timer(2);

  return r;
}

/* old style IRQ test */
int
legacy_irq_test()
{
  return 1;
}

int
fpga_irq_test()
{
  return 1;
}