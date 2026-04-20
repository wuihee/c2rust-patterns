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
  // ... (pattern-specific fields)
};

struct URLGlob {
  // [Pointer]
  struct URLPattern *pattern;

  // [Capacity]
  size_t palloc;

  // [Length]
  size_t pnum;
};

// Writes into the current slot pattern[pnum] without checking capacity.
// Relies on the caller (glob_parse) to have called add_glob() after each push,
// which is what actually does the capacity check and increments pnum.
static CURLcode glob_fixed(struct URLGlob *glob, char *fixed, size_t len) {
  // [Push Value: Precondition] — writes directly to pattern[pnum] assuming
  // a free slot exists. No capacity check here.
  struct URLPattern *pat = &glob->pattern[glob->pnum];
  pat->type = GLOB_SET;
  // ... (fill pat fields)
  (void)fixed;
  (void)len;
  return CURLE_OK;
}

// Commits the push: increments pnum, then grows if needed.
// Note: pnum is incremented BEFORE the capacity check, so the slot written
// by glob_fixed/glob_set/glob_range must already be valid.
static CURLcode add_glob(struct URLGlob *glob, size_t pos) {
  assert(glob->pattern[glob->pnum].type);

  // [Ensure Capacity] — checked after pnum increment, not before the write.
  if (++glob->pnum >= glob->palloc) {
    glob->palloc *= 2;
    struct URLPattern *np =
        realloc(glob->pattern, glob->palloc * sizeof(struct URLPattern));
    if (!np)
      return CURLE_OUT_OF_MEMORY;
    glob->pattern = np;
  }
  return CURLE_OK;
}

// Caller pattern: write into slot, then commit with add_glob.
// The two-step split means push validity depends on the caller always pairing
// a write function with add_glob — a precondition that is not locally enforced.
static CURLcode glob_parse(struct URLGlob *glob, const char *pattern,
                           curl_off_t *amount) {
  *amount = 1;
  while (*pattern) {
    // ... (parse one token from pattern into glob->buf)

    // [Push Value] via glob_fixed (or glob_set / glob_range)
    CURLcode result = glob_fixed(glob, NULL, 0);
    if (result)
      return result;

    // Capacity check and pnum increment happen here, after the write.
    result = add_glob(glob, 0);
    if (result)
      return result;
  }
  return CURLE_OK;
}
