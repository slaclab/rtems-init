/**
 * ----------------------------------------------------------------------------
 * Company    : SLAC National Accelerator Laboratory
 * ----------------------------------------------------------------------------
 * Description: Memory utilities for Cexpsh
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

#include <rtems/score/protectedheap.h>
#include <rtems/malloc.h>

#include <stdio.h>

extern Heap_Control _Workspace_Area;

void
workspaceUsage()
{
	Heap_Information_block info;
	_Protected_heap_Get_information(&_Workspace_Area, &info);

	printf("Workspace usage: %.2fK used, %.2fK free, %.2fK total\n", info.Used.total / 1024.,
		info.Free.total / 1024., (info.Free.total + info.Used.total) / 1024.f);
}

extern Heap_Control* RTEMS_Malloc_Heap;

void
mallocStats()
{
  Heap_Information_block info;
  if (!_Protected_heap_Get_information(RTEMS_Malloc_Heap, &info)) {
    printf("_Protected_heap_Get_information failed\n");
    return;
  }

  printf("Malloc Stats:\n");
  printf(" used:          %.2fK\n", info.Used.total / 1024.f);
  printf(" free:          %.2fK\n", info.Free.total / 1024.f);
	printf(" allocs:        %llu\n", (unsigned long long)info.Stats.allocs);
	printf(" frees:         %llu\n", (unsigned long long)info.Stats.frees);
	printf(" size:          %llu\n", (unsigned long long)info.Stats.size);
	printf(" free_size:     %llu\n", (unsigned long long)info.Stats.free_size);
	printf(" failed_allocs: %llu\n", (unsigned long long)info.Stats.failed_allocs);
	printf(" used_blocks:   %llu\n", (unsigned long long)info.Stats.used_blocks);
	printf(" free_blocks:   %llu\n", (unsigned long long)info.Stats.free_blocks);
	printf(" resizes:       %llu\n", (unsigned long long)info.Stats.resizes);
}
