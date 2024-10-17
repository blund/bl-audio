
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

typedef struct linear_allocator_t {
  void*    base;
  uint64_t current_offset;
  uint64_t size;
} linear_allocator_t;



void* _linalloc(linear_allocator_t* al, uint64_t size) {
  assert((uint64_t)(al->current_offset + size) < al->size);
  void* p = (void*)((uint64_t)al->base + al->current_offset);
  al->current_offset += size;

  return p;
}
