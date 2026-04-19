// repos/curl/lib/http2.c

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct h2_stream_ctx {
  // [Pointer]
  char **push_headers;

  // [Length]
  size_t push_headers_used;

  // [Capacity]
  size_t push_headers_alloc;
};

static int on_header(struct h2_stream_ctx *stream, const char *name,
                     const char *value) {
  if (!stream->push_headers) {
    stream->push_headers_alloc = 10;
    stream->push_headers = malloc(stream->push_headers_alloc * sizeof(char *));
    if (!stream->push_headers)
      return -1;
    stream->push_headers_used = 0;

    // [Ensure Capacity]
  } else if (stream->push_headers_used == stream->push_headers_alloc) {
    stream->push_headers_alloc *= 2;
    char **headp = realloc(stream->push_headers,
                           stream->push_headers_alloc * sizeof(char *));
    if (!headp)
      return -1;
    stream->push_headers = headp;
  }

  char *h = malloc(strlen(name) + strlen(value) + 2);
  if (!h)
    return -1;
  sprintf(h, "%s:%s", name, value);
  // [Push Operation: Single]
  stream->push_headers[stream->push_headers_used++] = h;
  return 0;
}
