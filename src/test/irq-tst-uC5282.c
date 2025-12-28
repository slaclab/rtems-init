/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: IRQ test for uC5282
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
#include <mcf5282/mcf5282.h>

#include <stdio.h>
#include <unistd.h>

#define PIT0_IRQ 0x77
#define PIT1_IRQ 0x78
#define PIT2_IRQ 0x79
#define PIT3_IRQ 0x7a

static int s_irqCnt = 0;

static struct pit_state {
  uint32_t pcsr;
  uint32_t pmr;
} pit_state[4];

/* configure a PIT instance for the test */
static void
conf_pit(int which)
{
  rtems_interrupt_level l;
  rtems_interrupt_disable(l);

  pit_state[which].pcsr = MCF5282_PIT_PCSR(which);
  pit_state[which].pmr = MCF5282_PIT_PMR(which);

  /* disable while we reconfigure */
  MCF5282_PIT_PCSR(which) &= ~MCF5282_PIT_PCSR_EN;

  MCF5282_PIT_PMR(which) = 0xFF;
  
  rtems_interrupt_enable(l);

  /* enable the timer with ~1.9 Hz operation */
  MCF5282_PIT_PCSR(which) = 
      MCF5282_PIT_PCSR_EN 
    | MCF5282_PIT_PCSR_PIE 
    | MCF5282_PIT_PCSR_RLD
    | MCF5282_PIT_PCSR_PRE(9) 
    | MCF5282_PIT_PCSR_OVW;
}

static void
restore_pit(int which)
{
  rtems_interrupt_level l;
  rtems_interrupt_disable(l);

  /* Disable while reconfiguring */
  MCF5282_PIT_PCSR(which) &= ~MCF5282_PIT_PCSR_EN;
  
  MCF5282_PIT_PMR(which) = pit_state[which].pmr;
  MCF5282_PIT_PCSR(which) = pit_state[which].pcsr;
  
  rtems_interrupt_enable(l);
}

static void
test_isr(void* param)
{
  uintptr_t n = (uintptr_t)param;
  s_irqCnt += n;
  
  /* clear the interrupt */
  MCF5282_PIT_PCSR(2) |= MCF5282_PIT_PCSR_PIF;
}

/* New style IRQ test using the rtems_interrupt API */
int
irq_test()
{
  fflush(stdout);
  fflush(stderr);
  sleep(1);

  int status = 0;
  rtems_status_code r;
  r = rtems_interrupt_handler_install(
    PIT2_IRQ,
    "PIT2 Test",
    RTEMS_INTERRUPT_UNIQUE,
    test_isr,
    (void*)0x2
  );

  if (r != RTEMS_SUCCESSFUL) {
    printf("irq_tst: %s\n", rtems_status_text(r));
    return -1;
  }

  r = rtems_interrupt_vector_enable(PIT2_IRQ);
  if (r != RTEMS_SUCCESSFUL) {
    printf("irq_test: %s\n", rtems_status_text(r));
    return -1;
  }

  conf_pit(2);

  int safeguard = 0;
  while (s_irqCnt < 20) {
    usleep(500000);
    safeguard++;
    if (safeguard > 60) { /* 30s timeout */
      printf("irq_tst: timeout!\n");
      status = -1;
      break;
    }
  }
  restore_pit(2);
  rtems_interrupt_vector_disable(PIT2_IRQ);
  return status;
}

/* old style IRQ test */
int
legacy_irq_test()
{
  return 1;
}

static int s_testFail = 0;

static void
vme_isr(void* arg, unsigned long vector)
{
  if (arg != (void*)0x1) {
    printk("arg != 0x1\n");
    s_testFail = 1;
  }

  if (vector != 200) {
    printk("vector != 200\n");
    s_testFail = 1;
  }

  printk("Got vme_isr arg=%p, vector=%ld\n", arg, vector);

  s_irqCnt++;
}

/* Special FPGA IRQs */
int
fpga_irq_test()
{
  /* Not easily possible to test currently! 
   * 'Fake VME' ISRs read the interrupts from external logic mapped using
   * the chip select module to 0x31000000. Since we have no peripherals on the
   * uCevolution dev board, we end up with exceptions instead. */
  return 1;

  printk("press IRQ1 now\n");
  fflush(stdout);
  fflush(stderr);
  sleep(1);

  s_irqCnt = 0;
  
  if (BSP_installVME_isr(200, vme_isr, (void*)0x1) < 0) {
    printk("BSP_installVME_isr failed\n");
    return -1;
  }
  
  int status = 0;

  conf_pit(2);
  
  int safe = 0;
  while (s_irqCnt < 1) {
    usleep(500000);
    safe++;

    if (safe > 60) {
      printf("fpga_irq_test: timeout\n");
      status = -1;
      break;
    }
  }

  restore_pit(2);

  return status;
}