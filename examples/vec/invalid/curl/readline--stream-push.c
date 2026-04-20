// repos/curl/tests/server/getpart.c

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  GPE_OK = 0,
  GPE_OUT_OF_MEMORY = 1,
  GPE_END_OF_FILE = 2,
} getpart_error;

static int readline(char **buffer,   // [Pointer]
                    size_t *bufsize, // [Capacity]
                    size_t *length,  // [Length]
                    FILE *stream) {
  size_t offset = 0;

  if (!*buffer) {
    *buffer = calloc(1, 128);
    if (!*buffer)
      return GPE_OUT_OF_MEMORY;
    *bufsize = 128;
  }

  for (;;) {
    int bytestoread = (int)(*bufsize - offset);

    // [Push Value: Stream] — reads from a FILE* directly into the buffer
    // instead of copying from another in-memory buffer.
    if (!fgets(*buffer + offset, bytestoread, stream)) {
      *length = 0;
      return (offset != 0) ? GPE_OK : GPE_END_OF_FILE;
    }

    *length = offset + strlen(*buffer + offset);
    if (*(*buffer + *length - 1) == '\n')
      break;
    offset = *length;
    if (*length < *bufsize - 1)
      continue;

    // [Ensure Capacity]
    char *newptr = realloc(*buffer, *bufsize * 2);
    if (!newptr)
      return GPE_OUT_OF_MEMORY;
    *buffer = newptr;
    *bufsize *= 2;
  }

  return GPE_OK;
}
