#include "../rtems-init.h"

#include <rtems/bspcmdline.h>
#include <stdio.h>

int
main(int argc, char** argv)
{
  printf("dns1=%s\n",dhcp_runtime_cfg.dns1);
  printf("bsp_cmdline=%s\n", rtems_bsp_cmdline_get());
  puts("Hello world!");
  return 0;
}

int
mulThem(int a, int b)
{
  printf("%d\n", a * b);
  return a * b;
}
