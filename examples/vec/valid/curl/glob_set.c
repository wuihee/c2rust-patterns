// repos/curl/src/tool_urlglob.c

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef long long curl_off_t;

typedef enum {
  CURLE_OK = 0,
  CURLE_URL_MALFORMAT = 3,
  CURLE_OUT_OF_MEMORY = 27,
} CURLcode;

#define FALSE false
#define TRUE true
#define FALLTHROUGH() ((void)0)
#define tool_safefree(x)                                                       \
  do {                                                                         \
    free(x);                                                                   \
    x = NULL;                                                                  \
  } while (0)

typedef enum {
  GLOB_SET = 1,
  GLOB_ASCII,
  GLOB_NUM,
} globtype;

struct dynbuf {
  char *bufr;    /* point to a null-terminated allocated buffer */
  size_t leng;   /* number of bytes *EXCLUDING* the null-terminator */
  size_t allc;   /* size of the current allocation */
  size_t toobig; /* size limit for the buffer */
#ifdef DEBUGBUILD
  int init; /* detect API usage mistakes */
#endif
};

struct URLPattern {
  globtype type;
  int globindex;
  union {
    struct {
      char **elem;
      size_t size;
      size_t idx;
      size_t palloc;
    } set;
  } c;
};

struct URLGlob {
  struct dynbuf buf;
  struct URLPattern *pattern;
  size_t palloc; /* number of pattern entries allocated */
  size_t pnum;   /* number of patterns used */
  char beenhere;
  const char *error; /* error message */
  size_t pos;        /* column position of error or 0 */
};

static CURLcode globerror(struct URLGlob *glob, const char *err, size_t pos,
                          CURLcode error) {
  glob->error = err;
  glob->pos = pos;
  return error;
}

static int multiply(curl_off_t *amount, curl_off_t with) {
  if (with > 0 && *amount > 0 && *amount > (((curl_off_t)-1) / with))
    return 1;
  *amount *= with;
  return 0;
}

static void *curlx_malloc(size_t size) { return malloc(size); }

static void *curlx_realloc(void *ptr, size_t size) {
  return realloc(ptr, size);
}

static char *curlx_strdup(const char *str) {
  size_t len = strlen(str) + 1;
  char *copy = malloc(len);
  if (copy)
    memcpy(copy, str, len);
  return copy;
}

static void curlx_free(void *ptr) { free(ptr); }

static char *curlx_dyn_ptr(const struct dynbuf *s) { return s->bufr; }

static void curlx_dyn_reset(struct dynbuf *s) {
  free(s->bufr);
  s->bufr = NULL;
  s->leng = 0;
  s->allc = 0;
}

static CURLcode curlx_dyn_addn(struct dynbuf *s, const void *mem, size_t len) {
  size_t fit = s->leng + len + 1;
  if (fit < s->leng)
    return CURLE_OUT_OF_MEMORY;
  if (fit > s->allc) {
    size_t newsize = s->allc ? s->allc : 16;
    while (newsize < fit)
      newsize *= 2;
    if (newsize > s->toobig && s->toobig)
      newsize = s->toobig;
    if (newsize < fit)
      return CURLE_OUT_OF_MEMORY;
    s->bufr = realloc(s->bufr, newsize);
    if (!s->bufr)
      return CURLE_OUT_OF_MEMORY;
    s->allc = newsize;
  }
  if (len)
    memcpy(s->bufr + s->leng, mem, len);
  s->leng += len;
  s->bufr[s->leng] = '\0';
  return CURLE_OK;
}

static CURLcode glob_set(struct URLGlob *glob, const char **patternp,
                         size_t *posp, curl_off_t *amount, int globindex) {
  /* processes a set expression with the point behind the opening '{'
     ','-separated elements are collected until the next closing '}'
   */
  struct URLPattern *pat;
  bool done = FALSE;
  const char *pattern = *patternp;
  const char *opattern = pattern;
  size_t opos = *posp - 1;
  CURLcode result = CURLE_OK;

  // [Length]
  size_t size = 0;

  // [Data Pointer]
  char **elem = NULL;

  // [Capacity]
  size_t palloc = 0; /* start with this */

  while (!done) {
    switch (*pattern) {
    case '\0': /* URL ended while set was still open */
      result = globerror(glob, "unmatched brace", opos, CURLE_URL_MALFORMAT);
      goto error;

    case '{':
    case '[': /* no nested expressions at this time */
      result = globerror(glob, "nested brace", *posp, CURLE_URL_MALFORMAT);
      goto error;

    case '}': /* set element completed */
      if (opattern == pattern) {
        result = globerror(glob, "empty string within braces", *posp,
                           CURLE_URL_MALFORMAT);
        goto error;
      }

      /* add 1 to size since it is to be incremented below */
      if (multiply(amount, size + 1)) {
        result = globerror(glob, "range overflow", 0, CURLE_URL_MALFORMAT);
        goto error;
      }
      done = TRUE;
      FALLTHROUGH();
    case ',':
      if (size >= 100000) {
        result = globerror(glob, "range overflow", 0, CURLE_URL_MALFORMAT);
        goto error;
      }

      // [Ensure Capcity]
      if (!palloc) {
        palloc = 5; /* a reasonable default */
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

      // [Push Value]
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

    case ']': /* illegal closing bracket */
      result = globerror(glob, "unexpected close bracket", *posp,
                         CURLE_URL_MALFORMAT);
      goto error;

    case '\\': /* escaped character, skip '\' */
      if (pattern[1]) {
        ++pattern;
        ++(*posp);
      }
      FALLTHROUGH();
    default:
      /* copy character to set element */
      if (curlx_dyn_addn(&glob->buf, pattern++, 1)) {
        result = CURLE_OUT_OF_MEMORY;
        goto error;
      }
      ++(*posp);
    }
  }

  *patternp = pattern; /* return with the new position */

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
  curlx_free(elem);
  return result;
}
