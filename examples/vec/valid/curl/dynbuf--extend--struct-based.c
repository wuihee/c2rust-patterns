// repos/curl/lib/curlx/dynbuf.c

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  CURLE_OK = 0,
  CURLE_OUT_OF_MEMORY = 27,
} CURLcode;

struct dynbuf {
  // [Pointer]
  char *bufr;

  // [Length]
  size_t leng;

  // [Capacity]
  size_t allc;
};

static CURLcode dyn_nappend(struct dynbuf *s, const unsigned char *mem,
                            size_t len) {
  size_t idx = s->leng;
  size_t a = s->allc;
  size_t fit = len + idx + 1;

  // [Ensure Capacity]
  if (!a) {
    a = fit < 32 ? 32 : fit;
  } else {
    while (a < fit)
      a *= 2;
  }
  if (a != s->allc) {
    void *p = realloc(s->bufr, a);
    if (!p)
      return CURLE_OUT_OF_MEMORY;
    s->bufr = p;
    s->allc = a;
  }

  // [Push Operation: Extend]
  if (len)
    memcpy(&s->bufr[idx], mem, len);
  s->leng = idx + len;
  s->bufr[s->leng] = 0;

  return CURLE_OK;
}
