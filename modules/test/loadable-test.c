#include <stdio.h>

void test_dynamic()
{
  printf("hello, world!\n");
}

int test_dynamic_puts(const char* s)
{
  puts(s);
  return 0;
}

void rtemsEntryPoint()
{
  printf("loadable-test.c: Loaded!\n");
}
