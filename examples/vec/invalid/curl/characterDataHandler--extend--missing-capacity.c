// repos/curl/docs/examples/xmlstream.c

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct MemoryStruct {
  // [Pointer]
  char *memory;

  // [Length]
  size_t size;

  // [Capacity: Missing] — no capacity field; realloc called unconditionally
  // to exact size + len + 1 on every handler invocation.
};

struct ParserStruct {
  int ok;
  struct MemoryStruct characters;
};

typedef char XML_Char;

static void characterDataHandler(void *userData, const XML_Char *s, int len) {
  struct ParserStruct *state = (struct ParserStruct *)userData;
  struct MemoryStruct *mem = &state->characters;

  // [Ensure Capacity: Missing]
  char *ptr = realloc(mem->memory, mem->size + (size_t)len + 1);
  if (!ptr) {
    state->ok = 0;
    return;
  }
  mem->memory = ptr;

  // [Push Value: Extend]
  memcpy(&(mem->memory[mem->size]), s, (size_t)len);
  mem->size += (size_t)len;
  mem->memory[mem->size] = 0;
}
