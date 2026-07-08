

/** HACK: random extern in RTEMS that causes rtems-ld to fail to link objects */
int __vectors = 0;

/* unlinkat is unsupported by newlib */
int __attribute__((visibility("default")))
unlinkat(int a, const char* b, int c)
{
  return -1;
}

int _GLOBAL_OFFSET_TABLE_ = 0;

extern char _PPC_INTERRUPT_DISABLE_MASK[];

/* The symbol ref scripts don't support conditionals, and they ref some cexp symbols.
 * As a workaround, we'll just define the symbols here, where we can case on cexp support */
#ifndef HAVE_CEXP
void* cexpModuleName;
void* cexpModuleInfo;
void* cexpModuleUnload;
#endif
