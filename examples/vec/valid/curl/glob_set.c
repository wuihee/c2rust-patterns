#include <stdlib.h>

struct dynbuf {
  char *bufr;    /* point to a null-terminated allocated buffer */
  size_t leng;   /* number of bytes *EXCLUDING* the null-terminator */
  size_t allc;   /* size of the current allocation */
  size_t toobig; /* size limit for the buffer */
#ifdef DEBUGBUILD
  int init; /* detect API usage mistakes */
#endif
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
  size_t size = 0;
  char **elem = NULL;
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
