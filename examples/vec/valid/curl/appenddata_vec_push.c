#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  GPE_OK = 0,
  GPE_OUT_OF_MEMORY = 1,
} getpart_error;

static int appenddata(char **dst_buf, size_t *dst_len, size_t *dst_alloc,
                      const char *src_buf, size_t src_len) {
  size_t need_alloc;

  if (!src_len) {
    return GPE_OK;
  }

  need_alloc = src_len + *dst_len + 1;

  // [Ensure Capacity]
  if (need_alloc > *dst_alloc) {
    size_t newsize = need_alloc * 2;
    char *newptr = realloc(*dst_buf, newsize);
    if (!newptr) {
      return GPE_OUT_OF_MEMORY;
    }
    *dst_alloc = newsize;
    *dst_buf = newptr;
  }

  // [Push Value]
  memcpy(*dst_buf + *dst_len, src_buf, src_len);
  *dst_len += src_len;
  *(*dst_buf + *dst_len) = '\0';
  return GPE_OK;
}
