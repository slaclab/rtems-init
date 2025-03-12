#include "../rtems-init.h"

#include <stdio.h>

int
main(int argc, char** argv)
{
  printf("cmdline=%s\n",dhcp_runtime_cfg.cmdline);
  puts("Hello world!");
  return 0;
}

int
mulThem(int a, int b)
{
  printf("%d\n", a * b);
  return a * b;
}
