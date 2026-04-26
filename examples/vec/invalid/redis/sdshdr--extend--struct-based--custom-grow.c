#include <stdint.h>
#include <string.h>

typedef char *sds;

struct __attribute__((__packed__)) sdshdr8 {
  uint8_t len;         /* [Length] used */
  uint8_t alloc;       /* [Capacity] excluding the header and null terminator */
  unsigned char flags; /* 3 lsb of type, 5 unused bits */
  char buf[];          /* [Vec Pointer] */
};
struct __attribute__((__packed__)) sdshdr16 {
  uint16_t len;        /* used */
  uint16_t alloc;      /* excluding the header and null terminator */
  unsigned char flags; /* 3 lsb of type, 5 unused bits */
  char buf[];
};
struct __attribute__((__packed__)) sdshdr32 {
  uint32_t len;        /* used */
  uint32_t alloc;      /* excluding the header and null terminator */
  unsigned char flags; /* 3 lsb of type, 5 unused bits */
  char buf[];
};
struct __attribute__((__packed__)) sdshdr64 {
  uint64_t len;        /* used */
  uint64_t alloc;      /* excluding the header and null terminator */
  unsigned char flags; /* 3 lsb of type, 5 unused bits */
  char buf[];
};

/* Enlarge the free space at the end of the sds string so that the caller
 * is sure that after calling this function can overwrite up to addlen
 * bytes after the end of the string, plus one more byte for nul term.
 * If there's already sufficient free space, this function returns without any
 * action, if there isn't sufficient free space, it'll allocate what's missing,
 * and possibly more:
 * When greedy is 1, enlarge more than needed, to avoid need for future reallocs
 * on incremental growth.
 * When greedy is 0, enlarge just enough so that there's free space for
 * 'addlen'.
 *
 * Note: this does not change the *length* of the sds string as returned
 * by sdslen(), but only the free buffer space we have. */
sds _sdsMakeRoomFor(sds s, size_t addlen, int greedy) {
  void *sh, *newsh;
  size_t avail = sdsavail(s);
  size_t len, newlen, reqlen;
  char type, oldtype = sdsType(s);
  int hdrlen;
  size_t bufsize, usable;
  int use_realloc;

  /* Return ASAP if there is enough space left. */
  if (avail >= addlen)
    return s;

  len = sdslen(s);
  sh = (char *)s - sdsHdrSize(oldtype);
  reqlen = newlen = (len + addlen);
  assert(newlen > len); /* Catch size_t overflow */
  // [Ensure Capacity]
  if (greedy == 1) {
    if (newlen < SDS_MAX_PREALLOC)
      newlen *= 2;
    else
      newlen += SDS_MAX_PREALLOC;
  }

  type = sdsReqType(newlen);

  /* Don't use type 5: the user is appending to the string and type 5 is
   * not able to remember empty space, so sdsMakeRoomFor() must be called
   * at every appending operation. */
  if (type == SDS_TYPE_5)
    type = SDS_TYPE_8;

  hdrlen = sdsHdrSize(type);
  assert(hdrlen + newlen + 1 > reqlen); /* Catch size_t overflow */
  use_realloc = (oldtype == type);
  if (use_realloc) {
    newsh = s_realloc_usable(sh, hdrlen + newlen + 1, &bufsize, NULL);
    if (newsh == NULL)
      return NULL;
    s = (char *)newsh + hdrlen;
    if (adjustTypeIfNeeded(&type, &hdrlen, bufsize)) {
      memmove((char *)newsh + hdrlen, s, len + 1);
      s = (char *)newsh + hdrlen;
      s[-1] = type;
      sdssetlen(s, len);
    }
  } else {
    /* Since the header size changes, need to move the string forward,
     * and can't use realloc */
    newsh = s_malloc_usable(hdrlen + newlen + 1, &bufsize);
    if (newsh == NULL)
      return NULL;
    adjustTypeIfNeeded(&type, &hdrlen, bufsize);
    memcpy((char *)newsh + hdrlen, s, len + 1);
    s_free(sh);
    s = (char *)newsh + hdrlen;
    s[-1] = type;
    sdssetlen(s, len);
  }
  usable = bufsize - hdrlen - 1;
  assert(type == SDS_TYPE_5 || usable <= sdsTypeMaxSize(type));
  sdssetalloc(s, usable);
  return s;
}

/* Append the specified binary-safe string pointed by 't' of 'len' bytes to the
 * end of the specified sds string 's'.
 *
 * After the call, the passed sds string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
sds sdscatlen(sds s, const void *t, size_t len) {
  size_t curlen = sdslen(s);

  s = sdsMakeRoomFor(s, len);
  if (s == NULL)
    return NULL;

  // [Push Value: Extend]
  memcpy(s + curlen, t, len);
  sdssetlen(s, curlen + len);
  s[curlen + len] = '\0';
  return s;
}
