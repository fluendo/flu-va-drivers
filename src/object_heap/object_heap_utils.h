#ifndef __FLU_VA_DRIVERS_OBJECT_HEAP_UTILS__
#define __FLU_VA_DRIVERS_OBJECT_HEAP_UTILS__

#include <assert.h>
#include "../ext/intel/intel-vaapi-drivers/object_heap.h"

void object_heap_clear (object_heap_p heap);
void object_heap_terminate (object_heap_p heap);

#endif