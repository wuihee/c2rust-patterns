// repos/curl/src/tool_urlglob.c

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef long long curl_off_t;

typedef enum {
  CURLE_OK = 0,
  CURLE_OUT_OF_MEMORY = 27,
  CURLE_URL_MALFORMAT = 3,
} CURLcode;

typedef enum { GLOB_SET = 1, GLOB_ASCII, GLOB_NUM } globtype;

struct URLPattern {
  globtype type;
  int globindex;
  union {
    struct {
      char **elem;
      curl_off_t size;
      curl_off_t idx;
      size_t palloc;
    } set;
    struct {
      int min, max, letter;
      unsigned char step;
    } ascii;
    struct {
      curl_off_t min, max, idx, step;
      int npad;
    } num;
  } c;
};

struct dynbuf {
  char *bufr;
  size_t leng;
  size_t allc;
  size_t toobig;
};

struct URLGlob {
  struct dynbuf buf;

  // [Pointer]
  struct URLPattern *pattern;

  // [Capacity]
  size_t palloc; /* number of pattern entries allocated */

  // [Length]
  size_t pnum; /* number of patterns used */

  char beenhere;
  const char *error;
  size_t pos;
};

#define curlx_malloc malloc
#define tool_safefree(p)                                                       \
  do {                                                                         \
    free(p);                                                                   \
    (p) = NULL;                                                                \
  } while (0)

static void *curlx_memdup0(const void *src, size_t len) {
  char *p = malloc(len + 1);
  if (p) {
    memcpy(p, src, len);
    p[len] = '\0';
  }
  return p;
}

static CURLcode globerror(struct URLGlob *glob, const char *err, size_t pos,
                          CURLcode error) {
  glob->error = err;
  glob->pos = pos;
  return error;
}

// Writes a single fixed string into the current slot pattern[pnum].
// Caller (glob_parse) subsequently calls add_glob() to commit the push.
static CURLcode glob_fixed(struct URLGlob *glob, char *fixed, size_t len) {
  // Missing ensure capacity?
  // [Push Value]
  struct URLPattern *pat = &glob->pattern[glob->pnum];

  pat->type = GLOB_SET;
  pat->globindex = -1;
  pat->c.set.size = 0;
  pat->c.set.idx = 0;
  pat->c.set.elem = curlx_malloc(sizeof(char *));

  if (!pat->c.set.elem)
    return globerror(glob, NULL, 0, CURLE_OUT_OF_MEMORY);

  pat->c.set.elem[0] = curlx_memdup0(fixed, len);
  if (!pat->c.set.elem[0]) {
    tool_safefree(pat->c.set.elem);
    return globerror(glob, NULL, 0, CURLE_OUT_OF_MEMORY);
  }

  pat->c.set.palloc = 1;
  pat->c.set.size = 1;
  return CURLE_OK;
}
