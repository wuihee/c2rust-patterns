// repos/curl/src/tool_urlglob.c

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

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

#define DEBUGASSERT(x) assert(x)
#define curlx_realloc realloc

static CURLcode globerror(struct URLGlob *glob, const char *err, size_t pos,
                          CURLcode error) {
  glob->error = err;
  glob->pos = pos;
  return error;
}

static CURLcode add_glob(struct URLGlob *glob, size_t pos) {
  DEBUGASSERT(glob->pattern[glob->pnum].type);

  // [Ensure Capacity]
  if (++glob->pnum >= glob->palloc) {
    struct URLPattern *np = NULL;

    glob->palloc *= 2;
    if (glob->pnum < 255) { /* avoid ridiculous amounts */
      np = curlx_realloc(glob->pattern,
                         glob->palloc * sizeof(struct URLPattern));
      if (!np)
        return globerror(glob, NULL, pos, CURLE_OUT_OF_MEMORY);
    } else
      return globerror(glob, "too many {} sets", pos, CURLE_URL_MALFORMAT);

    // [Push Value]: Pushes an empty value.
    glob->pattern = np;
  }
  return CURLE_OK;
}
