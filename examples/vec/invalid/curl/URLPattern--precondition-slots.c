// repos/curl/src/tool_urlglob.c

#include <assert.h>
#include <ctype.h>
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

#define DEBUGASSERT(x) assert(x)
#define curlx_malloc malloc
#define curlx_realloc realloc
#define curlx_strdup strdup
#define tool_safefree(p)                                                       \
  do {                                                                         \
    free(p);                                                                   \
    (p) = NULL;                                                                \
  } while (0)
#define FALLTHROUGH() /* fallthrough */
#define ISALPHA(c) isalpha((unsigned char)(c))
#define ISDIGIT(c) isdigit((unsigned char)(c))

static const char *curlx_dyn_ptr(const struct dynbuf *s) { return s->bufr; }
static size_t curlx_dyn_len(const struct dynbuf *s) { return s->leng; }
static void curlx_dyn_reset(struct dynbuf *s) { s->leng = 0; }
static int curlx_dyn_addn(struct dynbuf *s, const void *mem, size_t len) {
  size_t need = s->leng + len + 1;
  if (need > s->allc) {
    size_t newallc = s->allc ? s->allc * 2 : 32;
    while (newallc < need)
      newallc *= 2;
    char *p = realloc(s->bufr, newallc);
    if (!p)
      return 1;
    s->bufr = p;
    s->allc = newallc;
  }
  memcpy(s->bufr + s->leng, mem, len);
  s->leng += len;
  s->bufr[s->leng] = '\0';
  return 0;
}

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

/* Check if pattern starting at '[' is an IPv6 literal.
 * Sets *skip to the number of characters to skip (including brackets),
 * and *ipv6 to true if it is an IPv6 literal. */
static CURLcode peek_ipv6(const char *pattern, size_t *skip, bool *ipv6) {
  const char *p = pattern + 1; /* skip '[' */
  *skip = 0;
  *ipv6 = false;
  /* IPv6 literals contain ':' */
  while (*p && *p != ']') {
    if (*p == ':') {
      /* find closing ']' */
      while (*p && *p != ']')
        p++;
      if (*p == ']') {
        *skip = (size_t)(p - pattern) + 1;
        *ipv6 = true;
      }
      return CURLE_OK;
    }
    p++;
  }
  return CURLE_OK;
}

// Writes a single fixed string into the current slot pattern[pnum].
// Caller (glob_parse) subsequently calls add_glob() to commit the push.
static CURLcode glob_fixed(struct URLGlob *glob, char *fixed, size_t len) {
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

  (void)amount;

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

// Writes a range expression into the current slot pattern[pnum].
// Caller (glob_parse) subsequently calls add_glob() to commit the push.
static CURLcode glob_range(struct URLGlob *glob, const char **patternp,
                           size_t *posp, curl_off_t *amount, int globindex) {
  struct URLPattern *pat;
  const char *pattern = *patternp;

  (void)amount;

  // [Push Value]
  pat = &glob->pattern[glob->pnum];
  pat->globindex = globindex;

  if (ISALPHA(*pattern)) {
    bool pmatch = false;
    char min_c = 0;
    char max_c = 0;
    char end_c = 0;
    unsigned char step = 1;

    pat->type = GLOB_ASCII;

    if ((pattern[1] == '-') && pattern[2] && pattern[3]) {
      min_c = pattern[0];
      max_c = pattern[2];
      end_c = pattern[3];
      pmatch = true;

      if (end_c == ':') {
        /* parse step */
        pattern += 4;
      } else if (end_c != ']')
        pmatch = false;
      else
        pattern += 4;
    }

    *posp += (pattern - *patternp);

    if (!pmatch || !step || (min_c == max_c && step != 1) ||
        (min_c != max_c && (min_c > max_c)))
      return globerror(glob, "bad range", *posp, CURLE_URL_MALFORMAT);

    pat->c.ascii.step = step;
    pat->c.ascii.letter = pat->c.ascii.min = (unsigned char)min_c;
    pat->c.ascii.max = (unsigned char)max_c;
  } else if (ISDIGIT(*pattern)) {
    pat->type = GLOB_NUM;
    pat->c.num.npad = 0;
    pat->c.num.min = 0;
    pat->c.num.max = 0;
    pat->c.num.idx = pat->c.num.min;
    pat->c.num.step = 1;
    *posp += (pattern - *patternp);
  } else
    return globerror(glob, "bad range specification", *posp,
                     CURLE_URL_MALFORMAT);

  *patternp = pattern;
  return CURLE_OK;
}

static CURLcode glob_parse(struct URLGlob *glob, const char *pattern,
                           size_t pos, curl_off_t *amount) {
  /* processes a literal string component of a URL
     special characters '{' and '[' branch to set/range processing functions
   */
  CURLcode result = CURLE_OK;
  int globindex = 0; /* count "actual" globs */

  *amount = 1;

  /*
   * Goal: Parse `pattern` into a list of `URLPattern`s.
   *
   * Invariants:
   *
   * State:
   *
   * - `pattern` - The raw string being parsed.
   * - `glob` - The array of URL patterns we're trying to build.
   *
   * Transitions:
   *
   * - Add current character to buffer.
   * - Add buffer to list of patterns.
   */
  while (*pattern && !result) {

    // Keep adding current character to the glob->buf until we encounter '{'.
    while (*pattern && *pattern != '{') {
      if (*pattern == '[') {
        /* skip over IPv6 literals and [] */
        size_t skip = 0;
        bool ipv6;
        result = peek_ipv6(pattern, &skip, &ipv6);
        if (result)
          return result;
        if (!ipv6 && (pattern[1] == ']'))
          skip = 2;
        if (skip) {
          if (curlx_dyn_addn(&glob->buf, pattern, skip))
            return CURLE_OUT_OF_MEMORY;
          pattern += skip;
          continue;
        }
        break;
      }
      if (*pattern == '}' || *pattern == ']')
        return globerror(glob, "unmatched close brace/bracket", pos,
                         CURLE_URL_MALFORMAT);

      /* only allow \ to escape known "special letters" */
      if (*pattern == '\\' && (pattern[1] == '{' || pattern[1] == '[' ||
                               pattern[1] == '}' || pattern[1] == ']')) {

        /* escape character, skip '\' */
        ++pattern;
        ++pos;
      }
      /* copy character to literal */
      if (curlx_dyn_addn(&glob->buf, pattern++, 1))
        return CURLE_OUT_OF_MEMORY;
      ++pos;
    }

    if (curlx_dyn_len(&glob->buf)) {
      /* we got a literal string, add it as a single-item list */
      // curlx_dyn_ptr is a safety function.
      result = glob_fixed(glob, (char *)curlx_dyn_ptr(&glob->buf),
                          curlx_dyn_len(&glob->buf));
      if (!result)
        result = add_glob(glob, pos);
      curlx_dyn_reset(&glob->buf);
    } else {
      if (!*pattern) /* done */
        break;
      else if (*pattern == '{') {
        /* process set pattern */
        pattern++;
        pos++;
        result = glob_set(glob, &pattern, &pos, amount, globindex++);
        if (!result)
          result = add_glob(glob, pos);
      } else if (*pattern == '[') {
        /* process range pattern */
        pattern++;
        pos++;
        result = glob_range(glob, &pattern, &pos, amount, globindex++);
        if (!result)
          result = add_glob(glob, pos);
      }
    }
  }
  return result;
}
