// repos/curl/docs/examples/getinmemory.c

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct MemoryStruct {
  // [Pointer]
  char *memory;

  // [Length]
  size_t size;

  // [Capacity: Missing] — no capacity field; realloc called unconditionally
  // to exact size + realsize + 1 on every write.
};

static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  // [Ensure Capacity: Missing]
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr)
    return 0;
  mem->memory = ptr;

  // [Push Value: Extend]
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}
