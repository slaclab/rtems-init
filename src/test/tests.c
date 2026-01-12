/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: IRQ testing code
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

#include <stdio.h>
#include <unistd.h>

#include "common/util.h"

/* test function prototypes */
int legacy_irq_test();
int irq_test();

#ifdef LIBBSP_BEATNIK_BSP_H
# include "irq-tst-beatnik.c"
#elif defined(LIBBSP_M68K_UC5282_BSP_H)
# include "irq-tst-uC5282.c"
#elif defined(LIBBSP_POWERPC_MVME3100_BSP_H)
# include "irq-tst-mvme3100.c"
#else
int legacy_irq_test() { return 1; }
int irq_test() { return 1; }
#endif

struct test_entry {
  const char* name;
  int(*func)(void);
} TESTS[] = {
  {"Legacy IRQ", legacy_irq_test},
  {"IRQ", irq_test},
#ifdef LIBBSP_M68K_UC5282_BSP_H
  {"FPGA IRQs", fpga_irq_test},
#endif
};

void
run_tests()
{
  int passed = 0, failed = 0, skipped = 0, total = 0;

  for (int i = 0; i < sizeof(TESTS)/sizeof(TESTS[0]); ++i, ++total) {
    printf(
      ANSI_GREEN "[%-15s]" ANSI_RESET "...",
      TESTS[i].name
    );
    
    int r = TESTS[i].func();
    if (r > 0) {
      printf("Skipped\n");
      skipped++;
    }
    else if (r < 0) {
      printf(ANSI_RED "FAILED\n" ANSI_RESET);
      failed++;
    }
    else {
      printf(ANSI_GREEN "PASSED\n" ANSI_RESET);
      passed++;
    }
  }
  
  if (failed == 0) {
    printf(
      ANSI_GREEN "%d passed, %d failed, %d skipped, %d total\n" ANSI_RESET,
      passed,
      failed,
      skipped,
      total
    );
  }
  else {
    printf(
      ANSI_RED "%d passed, %d failed, %d skipped, %d total\n" ANSI_RESET,
      passed,
      failed,
      skipped,
      total
    );
  }
}