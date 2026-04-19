// repos/curl/lib/

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct mem {
  // [Pointer]
  char *buf;

  // [Length]
  size_t len;

  // [Capacity]
  size_t allocsize;
};

// [Custom Grow] — expands capacity to fit `needed` more bytes. Returns -1 on
// failure, `needed` on success.
static int mem_need(struct mem *mem, size_t needed) {
  if (needed > (unsigned)INT_MAX)
    return -1;

  if (needed <= (mem->allocsize - mem->len))
    return (int)needed;

  size_t newsize = needed < 4096 ? 4096 : needed;
  newsize += mem->len;

  char *newbuf = realloc(mem->buf, newsize);
  if (!newbuf)
    return -1;

  mem->buf = newbuf;
  mem->allocsize = newsize;
  return (int)needed;
}

static int mem_addn(struct mem *mem, const char *buf, size_t len) {
  // [Ensure Capacity]
  if (mem_need(mem, len + 1) < 0)
    return -1;

  // [Push Operation: Extend]
  memcpy(mem->buf + mem->len, buf, len);
  mem->len += len;
  mem->buf[mem->len] = '\0';
  return (int)len;
}
