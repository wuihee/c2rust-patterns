// repos/curl/lib/curlx/dynbuf.c

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MIN_FIRST_ALLOC 32
#define DYNINIT 0xbee51da /* random pattern */

typedef enum {
  CURLE_OK = 0,
  CURLE_OUT_OF_MEMORY = 27,
  CURLE_TOO_LARGE = 100,
} CURLcode;

struct dynbuf {
  // [Pointer]
  char *bufr; /* point to a null-terminated allocated buffer */

  // [Length]
  size_t leng; /* number of bytes *EXCLUDING* the null-terminator */

  // [Capacity]
  size_t allc;   /* size of the current allocation */
  size_t toobig; /* size limit for the buffer */
  int init;      /* detect API usage mistakes */
};

/*
 * free the buffer and re-init the necessary fields. It does not touch the
 * 'init' field and thus this buffer can be reused to add data to again.
 */
void curlx_dyn_free(struct dynbuf *s) {
  assert(s);
  assert(s->init == DYNINIT);
  free(s->bufr);
  s->leng = s->allc = 0;
}

/*
 * Store/append an chunk of memory to the dynbuf.
 */
static CURLcode dyn_nappend(struct dynbuf *s, const unsigned char *mem,
                            size_t len) {
  size_t idx = s->leng;
  size_t a = s->allc;
  size_t fit = len + idx + 1; /* new string + old string + zero byte */

  /* try to detect if there is rubbish in the struct */
  assert(s->init == DYNINIT);
  assert(s->toobig);
  assert(idx < s->toobig);
  assert(!s->leng || s->bufr);
  assert(a <= s->toobig);
  assert(!len || mem);

  if (fit > s->toobig) {
    curlx_dyn_free(s);
    return CURLE_TOO_LARGE;
  } else if (!a) {
    assert(!idx);
    /* first invoke */

    // [Ensure Capacity]
    if (MIN_FIRST_ALLOC > s->toobig)
      a = s->toobig;
    else if (fit < MIN_FIRST_ALLOC)
      a = MIN_FIRST_ALLOC;
    else
      a = fit;
  } else {
    while (a < fit)
      a *= 2;
    if (a > s->toobig)
      /* no point in allocating a larger buffer than this is allowed to use */
      a = s->toobig;
  }
  if (a != s->allc) {
    void *p = realloc(s->bufr, a);
    if (!p) {
      curlx_dyn_free(s);
      return CURLE_OUT_OF_MEMORY;
    }

    s->bufr = p;
    s->allc = a;
  }

  // [Push Value]
  if (len)
    memcpy(&s->bufr[idx], mem, len);
  s->leng = idx + len;
  s->bufr[s->leng] = 0;

  return CURLE_OK;
}
