// repos/curl/src/tool_urlglob.c:86

#include <stdbool.h>
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
#define curlx_realloc realloc
#define curlx_strdup strdup
#define tool_safefree(p) do { free(p); (p) = NULL; } while(0)
#define FALLTHROUGH() /* fallthrough */

static const char *curlx_dyn_ptr(const struct dynbuf *s) { return s->bufr; }
static void curlx_dyn_reset(struct dynbuf *s) { s->leng = 0; }
static int curlx_dyn_addn(struct dynbuf *s, const void *mem, size_t len) {
  size_t need = s->leng + len + 1;
  if (need > s->allc) {
    size_t newallc = s->allc ? s->allc * 2 : 32;
    while (newallc < need) newallc *= 2;
    char *p = realloc(s->bufr, newallc);
    if (!p) return 1;
    s->bufr = p;
    s->allc = newallc;
  }
  memcpy(s->bufr + s->leng, mem, len);
  s->leng += len;
  s->bufr[s->leng] = '\0';
  return 0;
}

static CURLcode globerror(struct URLGlob *glob, const char *err, size_t pos,
                          CURLcode error) {
  glob->error = err;
  glob->pos = pos;
  return error;
}

// Writes a set expression into the current slot pattern[pnum].
// Caller (glob_parse) subsequently calls add_glob() to commit the push.
static CURLcode glob_set(struct URLGlob *glob, const char **patternp,
                         size_t *posp, curl_off_t *amount, int globindex) {
  struct URLPattern *pat;
  bool done = false;
  const char *pattern = *patternp;
  const char *opattern = pattern;
  size_t opos = *posp - 1;
  CURLcode result = CURLE_OK;
  size_t size = 0;
  char **elem = NULL;
  size_t palloc = 0;

  while (!done) {
    switch (*pattern) {
    case '\0':
      result = globerror(glob, "unmatched brace", opos, CURLE_URL_MALFORMAT);
      goto error;

    case '{':
    case '[':
      result = globerror(glob, "nested brace", *posp, CURLE_URL_MALFORMAT);
      goto error;

    case '}':
      if (opattern == pattern) {
        result = globerror(glob, "empty string within braces", *posp,
                           CURLE_URL_MALFORMAT);
        goto error;
      }
      done = true;
      FALLTHROUGH();
    case ',':
      if (size >= 100000) {
        result = globerror(glob, "range overflow", 0, CURLE_URL_MALFORMAT);
        goto error;
      }

      if (!palloc) {
        palloc = 5;
        elem = curlx_malloc(palloc * sizeof(char *));
      } else if (size >= palloc) {
        char **arr = curlx_realloc(elem, palloc * 2 * sizeof(char *));
        if (!arr) {
          result = globerror(glob, NULL, 0, CURLE_OUT_OF_MEMORY);
          goto error;
        }
        elem = arr;
        palloc *= 2;
      }

      if (!elem) {
        result = globerror(glob, NULL, 0, CURLE_OUT_OF_MEMORY);
        goto error;
      }

      elem[size] = curlx_strdup(
          curlx_dyn_ptr(&glob->buf) ? curlx_dyn_ptr(&glob->buf) : "");
      if (!elem[size]) {
        result = globerror(glob, NULL, 0, CURLE_OUT_OF_MEMORY);
        goto error;
      }
      ++size;
      curlx_dyn_reset(&glob->buf);

      ++pattern;
      if (!done)
        ++(*posp);
      break;

    case ']':
      result = globerror(glob, "unexpected close bracket", *posp,
                         CURLE_URL_MALFORMAT);
      goto error;

    case '\\':
      if (pattern[1]) {
        ++pattern;
        ++(*posp);
      }
      FALLTHROUGH();
    default:
      if (curlx_dyn_addn(&glob->buf, pattern++, 1)) {
        result = CURLE_OUT_OF_MEMORY;
        goto error;
      }
      ++(*posp);
    }
  }

  *patternp = pattern;

  // Missing ensure capacity?
  // [Push Value]
  pat = &glob->pattern[glob->pnum];

  pat->type = GLOB_SET;
  pat->globindex = globindex;
  pat->c.set.elem = elem;
  pat->c.set.size = size;
  pat->c.set.idx = 0;
  pat->c.set.palloc = palloc;

  return CURLE_OK;
error: {
  size_t i;
  for (i = 0; i < size; i++)
    tool_safefree(elem[i]);
}
  free(elem);
  return result;
}
