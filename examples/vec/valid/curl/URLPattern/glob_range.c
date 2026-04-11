// repos/curl/src/tool_urlglob.c

#include <ctype.h>
#include <stdbool.h>
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

#define ISALPHA(c) isalpha((unsigned char)(c))
#define ISDIGIT(c) isdigit((unsigned char)(c))

static CURLcode globerror(struct URLGlob *glob, const char *err, size_t pos,
                          CURLcode error) {
  glob->error = err;
  glob->pos = pos;
  return error;
}

// Writes a range expression into the current slot pattern[pnum].
// Caller (glob_parse) subsequently calls add_glob() to commit the push.
static CURLcode glob_range(struct URLGlob *glob, const char **patternp,
                           size_t *posp, curl_off_t *amount, int globindex) {
  struct URLPattern *pat;
  const char *pattern = *patternp;

  // Missing ensure capacity?
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
