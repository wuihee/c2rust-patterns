struct mem {
  /* 'buf' points to memory contents that is always null-terminated so that it
     can be treated like a string if appropriate. 'recent' points to the most
     recent data written to 'buf'. */
  // [Vec Pointer]
  char *buf, *recent;
  /* 'len' and 'allocsize' are the length and allocated size of 'buf' */
  // [Length, Capacity]
  size_t len, allocsize;
};

/* expand free buffer space to needed size. return -1 or 'needed'. */
static int mem_need(struct mem *mem, size_t needed) {
  char *newbuf;
  size_t newsize;

  // [Capacity Check]
  if (needed > (unsigned)INT_MAX)
    return -1;

  if (needed <= (mem->allocsize - mem->len))
    return (int)needed;

  /* min 4k makes reallocations much less frequent when lengths are small */
  newsize = needed < 4096 ? 4096 : needed;

  newsize += mem->len;

  if (newsize < mem->len || newsize > (unsigned)INT_MAX)
    return -1;

  newbuf = realloc(mem->buf, newsize);

  if (!newbuf)
    return -1;

  if (mem->recent && mem->buf != newbuf)
    mem->recent = newbuf + (mem->recent - mem->buf);

  mem->buf = newbuf;
  mem->allocsize = newsize;

  return (int)needed;
}

static int mem_addn(struct mem *mem, const char *buf, size_t len) {
  // [Capacity Check]
  if (len + 1 < len || mem_need(mem, len + 1) < 0)
    return -1;

  mem->recent = mem->buf + mem->len;

  // [Push Value: Extend]
  memcpy(mem->recent, buf, len);
  mem->len += len;
  mem->buf[mem->len] = '\0';
  return (int)len;
}
