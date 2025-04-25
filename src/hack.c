

/** HACK: random extern in RTEMS that causes rtems-ld to fail to link objects */
int __vectors = 0;

int __attribute__((visibility("default"))) unlinkat(int a, const char* b, int c)
{
    return -1;
}

int _GLOBAL_OFFSET_TABLE_ = 0;