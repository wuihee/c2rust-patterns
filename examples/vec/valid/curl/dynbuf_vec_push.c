#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MIN_FIRST_ALLOC 32

typedef enum {
  CURLE_OK = 0,
  CURLE_OUT_OF_MEMORY = 27,
  CURLE_TOO_LARGE = 100,
} CURLcode;

typedef struct {
  char *bufr;
  size_t leng;
  size_t allc;
  size_t toobig;
} dynbuf;

static CURLcode dyn_nappend(dynbuf *s, const unsigned char *mem, size_t len) {
  size_t idx = s->leng;
  size_t a = s->allc;
  size_t fit = len + idx + 1;

  if (fit > s->toobig) {
    free(s->bufr);
    s->bufr = NULL;
    s->leng = 0;
    s->allc = 0;
    return CURLE_TOO_LARGE;
  }

  if (!a) {
    if (MIN_FIRST_ALLOC > s->toobig) {
      a = s->toobig;
    } else if (fit < MIN_FIRST_ALLOC) {
      a = MIN_FIRST_ALLOC;
    } else {
      a = fit;
    }
  } else {
    while (a < fit) {
      a *= 2;
    }
    if (a > s->toobig) {
      a = s->toobig;
    }
  }

  if (a != s->allc) {
    void *p = realloc(s->bufr, a);
    if (!p) {
      free(s->bufr);
      s->bufr = NULL;
      s->leng = 0;
      s->allc = 0;
      return CURLE_OUT_OF_MEMORY;
    }
    s->bufr = (char *)p;
    s->allc = a;
  }

  if (len) {
    memcpy(&s->bufr[idx], mem, len);
  }
  s->leng = idx + len;
  s->bufr[s->leng] = 0;
  return CURLE_OK;
}
