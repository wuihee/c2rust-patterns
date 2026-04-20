// repos/curl/tests/server/rtspd.c

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct rtspd_httprequest {
  // [Pointer]
  char *rtp_buffer;

  // [Length]
  size_t rtp_buffersize;

  // [Capacity: Missing] — no capacity field; each extend reallocs to exact
  // rtp_buffersize + rtp_size + 4 with no doubling strategy.
};

static void append_rtp_packet(struct rtspd_httprequest *req,
                              const char *rtp_scratch, int rtp_size) {
  if (!req->rtp_buffer) {
    // [Initialize] — first packet: take ownership of scratch buffer directly.
    req->rtp_buffer = (char *)rtp_scratch;
    req->rtp_buffersize = (size_t)rtp_size + 4;
  } else {
    // [Ensure Capacity: Missing] — reallocs to exact current + new size,
    // no capacity tracking or growth factor.
    char *rtp_buffer =
        realloc(req->rtp_buffer, req->rtp_buffersize + (size_t)rtp_size + 4);
    if (!rtp_buffer)
      return;
    req->rtp_buffer = rtp_buffer;

    // [Push Value: Extend]
    memcpy(req->rtp_buffer + req->rtp_buffersize, rtp_scratch,
           (size_t)rtp_size + 4);
    req->rtp_buffersize += (size_t)rtp_size + 4;
  }
}
