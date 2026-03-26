// Demo extraction for the stream_read_to_buffer pattern.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char *read_all(FILE *f, size_t *out_len) {
  unsigned char *buf = (unsigned char *)malloc(1024);
  size_t capacity = 1024;
  size_t len = 0;
  unsigned char temp[1024];

  if (!buf)
    return NULL;

  while (1) {
    size_t n = fread(temp, 1, sizeof(temp), f);
    if (n == 0) {
      if (ferror(f)) {
        free(buf);
        return NULL;
      }
      break;
    }

    while (len + n > capacity) {
      size_t new_capacity = capacity * 2;
      unsigned char *new_buf = (unsigned char *)realloc(buf, new_capacity);
      if (!new_buf) {
        free(buf);
        return NULL;
      }
      buf = new_buf;
      capacity = new_capacity;
    }

    memcpy(buf + len, temp, n);
    len += n;
  }

  *out_len = len;
  return buf;
}
