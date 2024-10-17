
#include <stdint.h>

typedef struct linear_allocator_t {
  void*    base;
  uint64_t current_offset;
  uint64_t size;

} linear_allocator_t;


#define LINALLOC(name) void* name(uint64_t size)
void* _linalloc(linear_allocator_t* al, uint64_t size);
