#include <rtems.h>
#include <bsp.h>
#include <bsp/irq.h>
#include <bsp/irq-generic.h>

#include <stdio.h>
#include <unistd.h>

#define RTC_IRQ (BSP_IRQ_GPP_0 + 3)
#define WDNMI_IRQ (BSP_IRQ_GPP_0 + 6)
#define TEST_IRQ RTC_IRQ

static int s_counter = 0;

#ifdef BSP_beatnik
#define RTC_BASE_ADDR 0xF1110000
static struct rtc_save {
  uint8_t data[4];
} rtc_save;
#endif

static void
dummy_irq()
{
  ++s_counter;
#ifdef BSP_beatnik
  /* Read the FLAGS register to clear the interrupt */
  uint8_t fl = *(volatile uint8_t*)(RTC_BASE_ADDR + 0x7FF0);
#endif
}

/* Configure RTC settings for the test */
static void
config_rtc()
{
#ifdef BSP_beatnik
  /* Set AFE */
  uint8_t* addr = (uint8_t*)(RTC_BASE_ADDR + 0x7FF6);
  *addr |= 0x80;
  /* Set RPT1-4 to 1 for per-second alarm repeats */
  addr = (uint8_t*)(RTC_BASE_ADDR + 0x7FF2);
  for (int i = 0; i < 4; ++i, ++addr) {
    rtc_save.data[i] = *addr;
    *addr |= 0x80;
  }
#endif
}

/* Restore RTC settings to their previous values */
static void
restore_rtc()
{
#ifdef BSP_beatnik
  /* Clear AFE */
  uint8_t* addr = (uint8_t*)(RTC_BASE_ADDR + 0x7FF6);
  *addr &= ~0x80;
  addr = (uint8_t*)(RTC_BASE_ADDR + 0x7FF2);
  for (int i = 0; i < 4; ++i, ++addr)
    *addr = rtc_save.data[i];
#endif
}

/* Legacy style IRQ test.
 * Uses the old BSP_install_rtems_irq API
 */
int
legacy_irq_tst()
{
  int ret = 0;
  s_counter = 0;

  const rtems_irq_connect_data data = {
    .name = TEST_IRQ,
    .hdl = dummy_irq,
  };
  if (!BSP_install_rtems_irq_handler(&data)) {
    printf("BSP_install_rtems_irq_handler: failed\n");
    return 1;
  }
  
  config_rtc();
  
  if (BSP_irq_set_priority(TEST_IRQ, 2) != 0) {
    printf("BSP_irq_set_priority: failed\n");
    ret = 1;
  }
  
  sleep(10);

  restore_rtc();
  
  if (s_counter == 0) {
    printf("counter was not >0!\n");
    ret = 1;
  }

  if (!BSP_remove_rtems_irq_handler(&data)) {
    printf("BSP_remove_rtems_irq_handler: failed\n");
    return 1;
  }
  
  return ret;
}

static void
new_irq(void* arg)
{
  ++s_counter;
}

/* New style IRQ test using the rtems_interrupt API */
int
irq_tst()
{
  int ret = 0, r = 0;
  s_counter = 0;

  if (RTEMS_SUCCESSFUL != (r = rtems_interrupt_handler_install(TEST_IRQ, "TIM",
    RTEMS_INTERRUPT_SHARED, new_irq, NULL)))
  {
    printf("rtems_interrupt_handler_install: %d\n", r);
    return 1;
  }

  if (rtems_interrupt_set_priority(TEST_IRQ, 2) != RTEMS_SUCCESSFUL) {
    printf("rtems_interrupt_handler_remove: failed\n");
    ret = 1;
  }

  sleep(10);

  if (s_counter == 0) {
    printf("counter was not >0!\n");
    ret = 1;
  }

  if (rtems_interrupt_handler_remove(TEST_IRQ, new_irq, NULL) != 
    RTEMS_SUCCESSFUL)
  {
    printf("rtems_interrupt_handler_remove: failed\n");
    return 1;
  }

  return ret;
}