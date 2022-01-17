#include "object_heap_utils.h"

void
object_heap_clear (object_heap_p heap)
{
  object_base_p obj;
  object_heap_iterator iter;

  assert (heap);

  obj = object_heap_first (heap, &iter);
  while (obj) {
    object_heap_free (heap, obj);
    obj = object_heap_next (heap, &iter);
  }
}

void
object_heap_terminate (object_heap_p heap)
{
  object_heap_clear (heap);
  object_heap_destroy (heap);
}