
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "linalloc.h"

void* _linalloc(linear_allocator_t* al, uint64_t size) {
  //assert((al->current_offset + size) < al->size);
  void* p = (void*)((uint8_t*)al->base + al->current_offset);
  al->current_offset += size;

  return p;
}


